/*
*
* wave tracer
* Copyright  Shlomi Steinberg
*
* LICENSE: Creative Commons Attribution-NonCommercial 4.0 International
*
*/

#include <wt/interaction/fsd/fraunhofer/free_space_diffraction.hpp>
#include <wt/math/common.hpp>
#include <wt/math/util.hpp>

#include <wt/ads/ads.hpp>

#include <wt/math/intersect/misc.hpp>

using namespace wt;
using namespace wt::fraunhofer;


free_space_diffraction_t::free_space_diffraction_t(
        const ads::ads_t* ads,
        const fsd_sampler::fsd_sampler_t *fsd_sampler,
        const frame_t& frame,
        wavenumber_t k, f_t total_power,
        const elliptic_cone_t& beam,
        const ads::intersection_record_t::edges_container_t& edges,
        const beam::gaussian_wavefront_t& wave_function) noexcept 
    : k(k), frame(frame),
      fsd_sampler(fsd_sampler) 
{
    constexpr auto interaction_point = pqvec2_t{};
    const auto cross_section_ellipse = pqvec2_t{ wave_function.envelope() };
    const auto r = m::max_element(cross_section_ellipse);
    const auto max_edge_length = f_t(.33) * r;

    aperture.recp_I = total_power>0? 1/total_power : 0;
    aperture.reserve(edges.size()*3);
    
    f_t P_total = 0;    // accumulates power total (all edges AND 0-th order lobe)

    // build edges
    for (const auto& ed : edges) {
        const auto& edge = ads->edge(ed);

        // only care about projected silhouette
        if (m::dot(beam.d(),edge.n1) * m::dot(beam.d(),edge.n2) >=0)
            continue;

        const pqvec2_t u1 = pqvec2_t{ frame.to_local(edge.a-beam.o()) },
                       u2 = pqvec2_t{ frame.to_local(edge.b-beam.o()) };

        // clamp points to cross section ellipse
        f_t t1=0, t2=1;
        if (!util::is_point_in_ellipsoid(pqvec2_t{ u1 }, cross_section_ellipse, pqvec2_t{}) ||
            !util::is_point_in_ellipsoid(pqvec2_t{ u2 }, cross_section_ellipse, pqvec2_t{})) {
            const auto intr = intersect::intersect_edge_ellipse(pqvec2_t{ u1 },pqvec2_t{ u2 }, 
                                                                cross_section_ellipse.x, cross_section_ellipse.y);
            if (intr.points==0)
                continue;
            t1 = m::max<f_t>(0,intr.t1);
            t2 = m::min<f_t>(1,intr.t2);
        }

        // subdivide edge into segments
        const auto len = m::length(pqvec2_t{ m::mix(u1,u2,t1) - m::mix(u1,u2,t2) });
        const auto segments = m::max(1,int(m::round(u::to_num(len/max_edge_length))+f_t(.5)));
        const auto seg = f_t(1)/segments;

        auto v1 = m::mix(u1,u2,t1);
        auto a  = wave_function.amplitude_magnitude(pqvec2_t{ v1 });
        auto ca = a;//std::polar(a, u::to_num(k*v1.z));

        for (int i=0;i<segments;++i) {
            const auto tt = m::mix(t1,t2,f_t(i+1)*seg);
            
            const auto v2 = m::mix(u1,u2,tt);
            const auto b  = wave_function.amplitude_magnitude(pqvec2_t{ v2 });
            const auto cb = b;//std::polar(b, u::to_num(k*v2.z));

            if (a>0 || b>0) {
                const auto v = pqvec2_t{ v1+v2 }/f_t(2) - interaction_point;
                const auto e = pqvec2_t{ v2-v1 };
                const auto edge = fsd::edge_t{ 
                    vec2_t{ e / fsd_unit }, 
                    vec2_t{ v / fsd_unit }, 
                    ca-cb, c_t{ 0,1 }*(ca+cb)/f_t(2)
                };

                const auto edge_pdf = fsd::Pj(edge);

                if (edge_pdf>0) {
                    aperture.edges.emplace_back(edge);
                    aperture.edge_pdfs.emplace_back(edge_pdf);

                    P_total += edge_pdf;
                }
            }

            v1=v2; a=b; ca=cb;
        }
    }

    // power in 0-th order lobe
    // TODO: horrible hack, to be replaced ASAP. This poor approximation is an alternative to numerically integrating the electric flux over the aperture.
    const auto psi0r = 3*fsd::P0_sigma;
    aperture.psi02 = (fsd::ASF_unclamped(aperture, psi0r * vec2_t{ -m::inv_sqrt_two,-m::inv_sqrt_two }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{ -1, 0 }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{ -m::inv_sqrt_two,+m::inv_sqrt_two }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{  0,+1 }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{ +m::inv_sqrt_two,+m::inv_sqrt_two }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{ +1, 0 }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{ +m::inv_sqrt_two,-m::inv_sqrt_two }) +
                      fsd::ASF_unclamped(aperture, psi0r * vec2_t{  0,-1 })
                     ) / f_t(8);
    aperture.P0 = fsd::P0(aperture) / m::sqr(u::to_num(k * fsd_unit));
    P_total += aperture.P0;

    if (P_total>0) {
        const auto recp_P_total = f_t(1) / P_total;
        aperture.P0_pdf = aperture.P0 * recp_P_total;
        for (auto& pdf : aperture.edge_pdfs)
            pdf *= recp_P_total;
    } else {
        aperture.P0_pdf = 1;
        aperture.edges = {};
    }
}
