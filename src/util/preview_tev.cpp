/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <string>
#include <vector>
#include <map>

#include <stdexcept>

#include <wt/math/common.hpp>
#include <wt/bitmap/bitmap.hpp>
#include <wt/sensor/response/tonemap/tonemap.hpp>

#include <wt/util/preview/preview_tev.hpp>

#include <wt/util/logger/logger.hpp>

#define TINYCOLORMAP_WITH_GLM
#include <tinycolormap.hpp>

#include <sockpp/tcp_connector.h>

using namespace wt;


// From https://github.com/Tom94/tev
// By Thomas Muller
class IpcPacket {
public:
    enum EType : char {
        OpenImage = 0,
        ReloadImage = 1,
        CloseImage = 2,
        UpdateImage = 3,
        CreateImage = 4,
        UpdateImageV2 = 5, // Adds multi-channel support
        UpdateImageV3 = 6, // Adds custom striding/offset support
        OpenImageV2 = 7, // Explicit separation of image name and channel selector
        VectorGraphics = 8,
    };

    IpcPacket() noexcept = default;
    IpcPacket(const char* data, size_t length) {
        if (length <= 0) {
            throw std::runtime_error{"Cannot construct an IPC packet from no data."};
        }
        mPayload.assign(data, data+length);
    }

    const char* data() const noexcept {
        return mPayload.data();
    }

    size_t size() const noexcept {
        return mPayload.size();
    }

    EType type() const noexcept {
        // The first 4 bytes encode the message size.
        return (EType)mPayload[4];
    }

    struct ChannelDesc {
        std::string name;
        int64_t offset;
        int64_t stride;
    };

    void setUpdateImage(const std::string& imageName, bool grabFocus, const std::vector<ChannelDesc>& channelDescs, int32_t x, int32_t y, int32_t width, int32_t height, const std::vector<float>& image) {
        if (channelDescs.empty()) {
            throw std::runtime_error("UpdateImage IPC packet must have a non-zero channel count.");
        }

        int32_t nChannels = (int32_t)channelDescs.size();
        std::vector<std::string> channelNames(nChannels);
        std::vector<int64_t> channelOffsets(nChannels);
        std::vector<int64_t> channelStrides(nChannels);

        for (int32_t i = 0; i < nChannels; ++i) {
            channelNames[i] = channelDescs[i].name;
            channelOffsets[i] = channelDescs[i].offset;
            channelStrides[i] = channelDescs[i].stride;
        }

        OStream payload{ mPayload };
        payload << EType::UpdateImageV3;
        payload << grabFocus;
        payload << imageName;
        payload << nChannels;
        payload << channelNames;
        payload << x << y << width << height;
        payload << channelOffsets;
        payload << channelStrides;

        size_t nPixels = width * height;
        size_t imageSize = 0;
        for (int32_t c = 0; c < nChannels; ++c) {
            imageSize = m::max(imageSize, (size_t)(channelOffsets[c] + (nPixels-1) * channelStrides[c] + 1));
        }

        if (image.size() != imageSize) {
            throw std::runtime_error("UpdateImage IPC packet's data size does not match specified dimensions, offset, and stride. (Expected: {" + std::to_string(imageSize) + "})");
        }

        payload << image;
    }

    void setCreateImage(const std::string& imageName, bool grabFocus, int32_t width, int32_t height, int32_t nChannels, const std::vector<std::string>& channelNames) {
        if ((int32_t)channelNames.size() != nChannels) {
            throw std::runtime_error{ "CreateImage IPC packet's channel names size does not match number of channels." };
        }

        OStream payload{mPayload};
        payload << EType::CreateImage;
        payload << grabFocus;
        payload << imageName;
        payload << width << height;
        payload << nChannels;
        payload << channelNames;
    }

private:
    std::vector<char> mPayload;

    class OStream {
    public:
        OStream(std::vector<char>& data) : mData{data} {
            // Reserve space for an integer denoting the size
            // of the packet.
            *this << (uint32_t)0;
        }

        template <typename T>
        OStream& operator<<(const std::vector<T>& var) {
            for (auto&& elem : var) {
                *this << elem;
            }
            return *this;
        }

        OStream& operator<<(const std::string& var) {
            for (auto&& character : var) {
                *this << character;
            }
            *this << '\0';
            return *this;
        }

        OStream& operator<<(bool var) {
            if (mData.size() < mIdx + 1) {
                mData.resize(mIdx + 1);
            }

            mData[mIdx] = var ? 1 : 0;
            ++mIdx;
            updateSize();
            return *this;
        }

        template <typename T>
        OStream& operator<<(T var) {
            if (mData.size() < mIdx + sizeof(T)) {
                mData.resize(mIdx + sizeof(T));
            }

            *(T*)&mData[mIdx] = var;
            mIdx += sizeof(T);
            updateSize();
            return *this;
        }
    private:
        void updateSize() {
            *((uint32_t*)mData.data()) = (uint32_t)mIdx;
        }

        std::vector<char>& mData;
        std::size_t mIdx = 0;
    };
};


namespace preview_tev {

struct impl_t {
    const std::string& host;
    const std::int32_t port;

    sockpp::tcp_connector conn;
    bool connected = false;

    std::map<std::string, vec2u32_t> images;

    impl_t(const std::string& host, const std::int32_t port) : host(host), port(port) {
        sockpp::initialize();

        using namespace std::chrono;
        if (auto res = conn.connect(host, port, 10s); !res) {
            wt::logger::cerr() << "(preview_tev) Error connecting to: '" 
                                << host << ":" << std::format("{:d}",port) 
                                << "' --- " << res.error_message() << '\n';
            return;
        }

        connected = true;
    }

    bool create(std::string preview_id, std::map<std::string, vec2u32_t>::iterator it, vec2u32_t image_size) {
        if (!connected) {
            using namespace std::chrono;
            if (auto res = conn.connect(host, port, 10s); !res)
                return false;
            connected = true;
        }

        IpcPacket create;
        create.setCreateImage("wave_tracer '" + preview_id + "'", false, 
                            image_size.x, image_size.y, 3, 
                            { "R", "G", "B" });
        const auto write_result = conn.write_n(create.data(), create.size());

        if (write_result.is_error()) {
            // wt::logger::cerr() << "(preview_tev) Failed to write " << " --- " << write_result.error_message() << '\n';
            conn.close();
            return false;
        }

        images.insert_or_assign(it, std::move(preview_id), image_size);

        return true;
    }

    bool update(const std::string& preview_id, const std::vector<float>& surface, vec2u32_t image_size) {
        const auto pixels = surface.size()/3;

        IpcPacket update;
        update.setUpdateImage("wave_tracer '" + preview_id + "'", false, 
                            { 
                                IpcPacket::ChannelDesc{ .name="R", .offset=std::int64_t(pixels*0), .stride=1 },
                                IpcPacket::ChannelDesc{ .name="G", .offset=std::int64_t(pixels*1), .stride=1 },
                                IpcPacket::ChannelDesc{ .name="B", .offset=std::int64_t(pixels*2), .stride=1 },
                            }, 0, 0, image_size.x, image_size.y, 
                            surface);
        const auto write_result = conn.write_n(update.data(), update.size());

        if (write_result.is_error()) {
            // wt::logger::cerr() << "(preview_tev) Failed to write " << " --- " << write_result.error_message() << '\n';
            conn.close();
            connected = false;
            return false;
        }

        return true;
    }
};

}

preview_tev_t::preview_tev_t(const std::string& host, const std::int32_t port) {
    auto pimpl = new preview_tev::impl_t(host, port);
    this->ptr = std::shared_ptr<preview_tev::impl_t>(pimpl);
}

void preview_tev_t::update(const std::string& preview_id, 
                           sensor::developed_scalar_film_t<2>&& surface,
                           const f_t spp,
                           const sensor::response::tonemap_t* tonemap) const {
    auto pimpl = static_cast<preview_tev::impl_t*>(ptr.get());

    const auto image_size = surface.dimensions();
    const auto pixels = surface.total_pixels();
    const auto chnls = surface.components();
    assert(chnls==1 || chnls==3);

    auto it = pimpl->images.find(preview_id);
    if (!pimpl->connected || it==pimpl->images.end() || it->second!=image_size) {
        if (!pimpl->create(preview_id, it, image_size))
            return;
        it = pimpl->images.find(preview_id);
    }

    std::vector<float> image;
    image.resize(pixels*3);
    for (std::size_t p=0; p<pixels; ++p) {
        vec3_t val{};
        if (chnls==1)
            val = tonemap ? (*tonemap)(surface[p]) : vec3_t{ surface[p] };
        else if (chnls==3)
            val = tonemap ? 
                (*tonemap)(vec3_t{ surface[p*chnls+0],surface[p*chnls+1],surface[p*chnls+2] }) :
                vec3_t{ surface[p*chnls+0],surface[p*chnls+1],surface[p*chnls+2] };

        image[0*pixels+p] = val.x;
        image[1*pixels+p] = val.y;
        image[2*pixels+p] = val.z;
    }

    pimpl->update(preview_id, image, surface.dimensions());
}
