/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <memory>
#include <mutex>

#include <wt/util/seeded_mt19937_64.hpp>

#include <wt/scene/element/attributes.hpp>
#include <wt/scene/loader/node_readers.hpp>

#include <wt/sampler/sobolld.hpp>

#include <wt/sampler/sobolld/irreducible_gf3.hpp>
#include <wt/sampler/sobolld/sobolld_sampler.hpp>

#include <wt/util/logger/logger.hpp>

using namespace wt;
using namespace sampler;


static std::unique_ptr<sobolld::irreducible_gf3_t> irreducible_gf3;


struct sobolld_t::sobolld_impl_t {
    using sobol_sampler_t = sobolld::sobolls_sampler<47>;

    sobol_sampler_t sobol_sampler;

    sobolld_impl_t(const sobolld::irreducible_gf3_t& gf3)
        : sobol_sampler(sobol_sampler_t::max_mat_size(), gf3)
    {}
};


thread_local std::vector<f_t> sobolld_t::sobol_samples;
thread_local std::size_t sobolld_t::next_point_idx;


sobolld_t::sobolld_t(std::string id) 
    : sampler_t(std::move(id))
{}

sobolld_t::~sobolld_t() noexcept {}

void sobolld_t::generate_samples() const noexcept {
    static thread_local seeded_mt19937_64 rd;

    this->sobol_samples = pimpl->sobol_sampler.template generate_points<f_t>([&]() {
        return std::uniform_int_distribution<unsigned>{}(rd.engine());
    });
    this->next_point_idx = 0;
}


void sobolld_t::deferred_load(const wt::wt_context_t &context) {
    static std::mutex m;

    if (!irreducible_gf3) {
        std::unique_lock l(m);
        if (!irreducible_gf3) {
            const auto dat_path = context.resolve_path(std::filesystem::path{ "data" } / "sobolld" / "initIrreducibleGF3.dat");
            if (!dat_path)
                throw std::runtime_error("(sobolld) 'initIrreducibleGF3.dat' not found.");
            irreducible_gf3 = std::make_unique<sobolld::irreducible_gf3_t>(*dat_path);
        }
    }

    pimpl = std::make_unique<sobolld_impl_t>(*irreducible_gf3);
}


scene::element::info_t sobolld_t::description() const {
    using namespace scene::element;
    return info_for_scene_element(*this, "sobolld");
}

std::shared_ptr<sobolld_t> sobolld_t::load(std::string id, 
                                           scene::loader::loader_t* loader, 
                                           const scene::loader::node_t& node, 
                                           const wt::wt_context_t &context) {
    for (auto& item : node.children_view()) {
        logger::cwarn()
            << loader->node_description(item)
            << "(sobolld sampler loader) unqueried node type " << item.name() << " (\"" << item["name"] << "\")" << '\n';
    }

    auto ptr = std::make_shared<sobolld_t>(std::move(id));
    loader->enqueue_loading_task(ptr.get(), "sobolld", [p=ptr, &context]() { p->deferred_load(context); });

    return ptr;
}
