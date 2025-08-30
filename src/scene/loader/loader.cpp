/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <memory>
#include <future>
#include <string>
#include <vector>
#include <map>

#include <wt/scene/loader/loader.hpp>
#include <wt/scene/loader/node.hpp>

#include <wt/util/logger/logger.hpp>

#include <wt/bsdf/bsdf.hpp>
#include <wt/sampler/sampler.hpp>
#include <wt/sampler/uniform.hpp>
#include <wt/sensor/response/response.hpp>
#include <wt/spectrum/spectrum.hpp>
#include <wt/texture/texture.hpp>
#include <wt/texture/complex.hpp>
#include <wt/interaction/surface_profile/surface_profile.hpp>

#include <wt/version.hpp>
#include <wt/util/format/parse.hpp>

using namespace wt;
using namespace wt::scene::loader;


struct wt::scene::loader::loader_t::impl_t {
    node_t* scene_node;
    std::optional<progress_callback_t> callbacks;

    std::atomic<std::uint32_t> total_scene_tasks = 0;
    std::atomic<std::uint32_t> completed_scene_tasks = 0;
    std::atomic<std::uint32_t> total_resources_tasks = 0;
    std::atomic<std::uint32_t> completed_resources_tasks = 0;

    alignas(64) std::mutex shared_scene_elements_lock;
    std::map<std::string, shared_scene_element_task_t> shared_scene_element_tasks;

    std::future<std::shared_ptr<integrator::integrator_t>> integrator_task;
    std::future<std::shared_ptr<sampler::sampler_t>> sampler_task;
    std::vector<std::future<std::shared_ptr<sensor::sensor_t>>> sensors_tasks;
    std::vector<std::future<std::shared_ptr<shape_t>>> shapes_tasks;

    alignas(64) mutable std::mutex shapes_lock;
    std::vector<std::shared_ptr<shape_t>> shapes;
};


loader_t::loader_t(std::string scene_name,
                   const wt_context_t &ctx,
                   std::optional<progress_callback_t> callbacks)
    : context(ctx),
      name(std::move(scene_name))
{
    pimpl = std::make_shared<impl_t>();
    pimpl->callbacks = std::move(callbacks);
}

void loader_t::update_defaults(node_t& root, const defaults_defines_t& user_defines) {
    // Read defaults
    auto defines = user_defines;
    for (const auto& def : root.children("default")) {
        parse_default(*def, defines);
        root.erase_child(*def);
    }
    // Update XML with defines
    std::set<std::string> used_defines;
    set_defines(root, defines, &used_defines);

    // warn about unused defines
    for (const auto& d : user_defines) {
        if (!used_defines.contains(d.first))
            logger::cwarn() << "Unused user-supplied define \"" << d.first << "\"." << '\n';
    }
}

void loader_t::load(node_t* scene_node,
                    const defaults_defines_t& user_defines) {
    pimpl->scene_node = scene_node;
    this->success = true;

    // check version
    auto& root = *pimpl->scene_node;
    const auto version = parse_version(root["version"]);
    {
        wt_version_t wtvers;
        if (version.major!=wtvers.major() || version.minor>wtvers.minor())
            throw std::runtime_error("(scene XML) Unsupported scene version.");
    }

    // write defaults in nodes
    update_defaults(root, user_defines);
    

    /*
     * Load scene
     */

    // load scene elements
    std::unique_lock l(pimpl->shared_scene_elements_lock);
    
    for (auto& item : root.children_view()) {
        const auto& name = item.name();

        // dynamic toggling of elements
        bool skip_node = false;
        for (auto& enabled : item.children_view()) {
            if (enabled["name"]=="enabled") {
                try {
                    skip_node = !stob_strict(enabled["value"]);
                    item.erase_child(enabled);     // remove from xml
                } catch (const std::runtime_error& ex) {
                    wt::logger::cerr(verbosity_e::important) << node_description(item) << ex.what() << '\n';
                    this->success = false;
                }
                break;
            }
        }
        if (skip_node) continue;

        std::string id = item["id"];
        if (!id.length())
            id ="__unnamed_$" + std::format("{:d}",++unnamed_ids);
        if (all_ids.find(id) != all_ids.end()) {
            wt::logger::cerr(verbosity_e::important) << node_description(item) << "duplicate id \"" << id << "\"\n";
            this->success = false;
            continue;
        }
        all_ids.emplace(id, &item);

        const auto create_task = [this]<typename T>(std::string sid, 
                                                    const node_t* item) {
            ++pimpl->total_scene_tasks;
            return this->context.threadpool->enqueue([this, id=std::move(sid), item]() mutable {
                using R = decltype(T::load(id, this, *item, this->context));

                R ret = nullptr;
                try {
                    ret = T::load(std::move(id), this, *item, this->context);
                } catch (const std::runtime_error& ex) {
                    // see if we have a child node, that caused the exception
                    const auto* loader_ex = dynamic_cast<const scene_loading_exception_t*>(&ex);
                    const auto* errnode = loader_ex ? loader_ex->get_scene_loader_node() : item;

                    logger::cerr(verbosity_e::important) << node_description(*errnode) << ex.what() << '\n';
                    this->success = false;
                    ret = nullptr;
                }

                // progress
                const auto completed = ++pimpl->completed_scene_tasks;
                if (pimpl->callbacks && pimpl->callbacks->scene_loading_progress_update)
                    pimpl->callbacks->scene_loading_progress_update(completed / f_t(pimpl->total_scene_tasks.load()));

                return ret;
            });
        };

        if (name == bsdf::bsdf_t::scene_element_class() ||
            name == emitter::emitter_t::scene_element_class() ||
            name == sensor::response::response_t::scene_element_class() ||
            name == spectrum::spectrum_t::scene_element_class() ||
            name == texture::texture_t::scene_element_class() ||
            name == texture::complex_t::scene_element_class() ||
            name == surface_profile::surface_profile_t::scene_element_class()) {
            // shared scene elements
            auto idcopy = id;
            pimpl->shared_scene_element_tasks.emplace(
                std::move(id),
                create_task.template operator()<scene::scene_element_t>(std::move(idcopy), &item)
            );
        }
        else if (name == integrator::integrator_t::scene_element_class()) {
            if (pimpl->integrator_task.valid()) {
                wt::logger::cerr(verbosity_e::important)
                    << node_description(item) << "only one sampler must be provided!" << '\n';
                this->success = false;
            }
            pimpl->integrator_task = 
                        create_task.template operator()<integrator::integrator_t>(std::move(id), &item);
        }
        else if (name == sampler::sampler_t::scene_element_class()) {
            if (pimpl->sampler_task.valid()) {
                wt::logger::cerr(verbosity_e::important)
                    << node_description(item) << "only one sampler must be provided!" << '\n';
                this->success = false;
            }
            pimpl->sampler_task = 
                        create_task.template operator()<sampler::sampler_t>(std::move(id), &item);
        }
        else if (name == sensor::sensor_t::scene_element_class()) {
            // exceeding max sensor count?
            if (pimpl->sensors_tasks.size()==scene_t::max_supported_sensors) {
                wt::logger::cerr(verbosity_e::important)
                    << node_description(item) 
                    << "exceeding max allowed sensor count (" << std::format("{:L}",scene_t::max_supported_sensors) << ")"
                    << '\n';
                this->success = false;
            }

            pimpl->sensors_tasks.emplace_back(
                        create_task.template operator()<sensor::sensor_t>(std::move(id), &item));
        } 
        else if (name == shape_t::scene_element_class()) {
            pimpl->shapes_tasks.emplace_back(
                    create_task.template operator()<shape_t>(std::move(id), &item));
        } 
        else {
                wt::logger::cerr(verbosity_e::important)
                    << node_description(item) << "unknown node \"" << name << "\"" << '\n';
        }
    }
}

void loader_t::wait_shapes() const {
    std::unique_lock l(pimpl->shapes_lock);
    for (const auto &t : pimpl->shapes_tasks) t.wait();
}

std::vector<std::shared_ptr<shape_t>>& loader_t::get_shapes() {
    std::unique_lock l(pimpl->shapes_lock);
    for (auto &t : pimpl->shapes_tasks)
        if (auto p=t.get(); p)
            pimpl->shapes.emplace_back(std::move(p));
    pimpl->shapes_tasks.clear();
    return pimpl->shapes;
}

std::optional<loader_t::shared_scene_element_task_t> loader_t::get_shared_task(const std::string &id) {
    assert(id.length());
    std::unique_lock l(pimpl->shared_scene_elements_lock);

    const auto it = pimpl->shared_scene_element_tasks.find(id);
    if (it==pimpl->shared_scene_element_tasks.end())
        return std::nullopt;
    return it->second;
}

void loader_t::on_new_aux_task() noexcept {
    ++pimpl->total_resources_tasks;
}
void loader_t::on_completed_aux_task() noexcept {
    const auto completed = ++pimpl->completed_resources_tasks;
    if (pimpl->callbacks && pimpl->callbacks->resources_loading_progress_update)
        pimpl->callbacks->resources_loading_progress_update(completed / f_t(pimpl->total_resources_tasks.load()));
}

void loader_t::wait() const {
    wait_shapes();
    if (pimpl->sampler_task.valid()) pimpl->sampler_task.wait();
    if (pimpl->integrator_task.valid()) pimpl->integrator_task.wait();
    for (const auto &t : pimpl->shared_scene_element_tasks) t.second.wait();
    for (const auto &t : pimpl->sensors_tasks) t.wait();
    for (const auto &f : aux_tasks) f.second.wait();
}

std::unique_ptr<scene_t> loader_t::get() {
    if (!pimpl->integrator_task.valid())
        throw std::runtime_error("(scene XML) integrator not specified!");
    auto integrator = pimpl->integrator_task.get();

    std::vector<std::shared_ptr<emitter::emitter_t>> emitters;
    std::vector<std::shared_ptr<sensor::sensor_t>> sensors;
    std::shared_ptr<sampler::sampler_t> sampler;

    // wait for all shared elements
    for (;;) {
        std::unique_lock l(pimpl->shared_scene_elements_lock);
        if (pimpl->shared_scene_element_tasks.empty())
            break;
        auto f = std::move(pimpl->shared_scene_element_tasks.begin()->second);
        pimpl->shared_scene_element_tasks.erase(pimpl->shared_scene_element_tasks.begin());
        l.unlock();

        // is an emitter?
        auto& obj = f.get();
        if (auto p = std::dynamic_pointer_cast<emitter::emitter_t>(obj); p)
            emitters.emplace_back(std::move(p));
    }

    {
        // wait for sensors
        for (auto &t : pimpl->sensors_tasks)
            if (auto p=t.get(); p)
                sensors.emplace_back(std::move(p));
        pimpl->sensors_tasks.clear();

        // and sampler
        if (!pimpl->sampler_task.valid())
            sampler = std::make_shared<sampler::uniform_t>("default_sampler");
        else
            sampler = pimpl->sampler_task.get();
    }

    get_shapes();
    // add area emitters to list of emitters
    for (const auto& s : pimpl->shapes) {
        if (const auto& e = s->get_emitter(); e)
            emitters.emplace_back(e);
    }

    {
        std::unique_lock l(aux_loadable_lock);
        for (;;) {
            if (aux_tasks.empty()) break;
            const auto* res = aux_tasks.begin()->first;
            l.unlock();

            // wait for all tasks for this resource to complete, and erase the tasks
            complete_loading_tasks_for_resource(res);
            l.lock();
            aux_tasks.erase(res);
        }
    }

    // exceptions should have been rethrown by now
    // double check
    if (has_errors()) {
        // on failure callback
        if (pimpl->callbacks && pimpl->callbacks->on_terminate)
            pimpl->callbacks->on_terminate();
        throw std::runtime_error("(scene XML) failed loading scene");
    }
    
    // on success callback
    if (pimpl->callbacks && pimpl->callbacks->on_finish)
        pimpl->callbacks->on_finish();
    
    auto ret = std::make_unique<scene_t>(
        this->get_name(),
        this->context,
        std::move(integrator), 
        std::move(sensors),
        std::move(sampler),
        std::move(emitters),
        pimpl->shapes
    );

    return ret;
}

loader_t::version_t loader_t::parse_version(const std::string &vers) {
    std::stringstream ss(vers);
    version_t version;

    ss >> version.major;
    if (ss.peek() == '.') {
        ss.ignore();
        ss >> version.minor;
        if (ss.peek() == '.') {
            ss.ignore();
            ss >> version.patch;
        }
    }

    return version;
}

void loader_t::set_defines(node_t& node,
                           const defaults_defines_t& defines,
                           std::set<std::string>* used_defines) {
    // Overwrite variables with defines in XML
    for (auto& item : node.children()) {
        for (auto &attr : item->attributes()) {
            auto str = attr.second;
            for (std::size_t idx=0;idx<str.size();) {
                idx = str.find('$', idx);
                if (idx == std::string::npos) break;
                if (idx>0 && str[idx-1]=='\\') { str.erase(idx-1); continue; };

                const auto end = std::find_if_not(str.begin()+idx+1, str.end(), [](char const &c) {
                    return isalnum(c) || c=='_';
                }) - str.begin() - 1;
                const auto name = str.substr(idx+1, end-idx);

                // find define
                const auto it = defines.find(name);
                if (it==defines.end()) {
                    logger::cerr(verbosity_e::important) << "Unknown define \"" << attr.second << "\"." << '\n';
                    idx = str.size();
                    continue;
                }
                // replace value in node
                str.replace(idx, end-idx+1, it->second);
                // mark define as used
                if (used_defines)
                    used_defines->emplace(name);
            }
            
            if (!item->set_attribute(attr.first, str))
                throw scene_loading_exception_t("(scene XML) Failed updating defines", *item);
        }

        set_defines(*dynamic_cast<node_t*>(item.get()), defines, used_defines);
    }
}

void loader_t::parse_default(const node_t& node, defaults_defines_t& defines) {
    const std::string& name = node["name"];
    if (!defines.contains(name))
        defines[name] = node["value"];
}
