/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_SSE_ODE_RK43INTEGRATORSSE_HPP
#define BI_SSE_ODE_RK43INTEGRATORSSE_HPP

namespace bi {
/**
 * @copydoc RK43Integrator
 */
template<class B, class S, class T1>
class RK43IntegratorSSE {
public:
  /**
   * @copydoc RK43Integrator::integrate()
   */
  static void update(const T1 t1, const T1 t2, State<B,ON_HOST>& s);
};
}

#include "../sse_state.hpp"
#include "../sse_host.hpp"
#include "../sse_const_host.hpp"
#include "../sse_shared_host.hpp"
#include "../../host/ode/RK43VisitorHost.hpp"
#include "../../host/ode/IntegratorConstants.hpp"
#include "../../state/Pa.hpp"
#include "../../typelist/front.hpp"
#include "../../typelist/pop_front.hpp"

template<class B, class S, class T1>
void bi::RK43IntegratorSSE<B,S,T1>::update(const T1 t1, const T1 t2,
    State<B,ON_HOST>& s) {
  /* pre-condition */
  assert (t1 < t2);

  typedef Pa<ON_HOST,B,sse_real,sse_const_host,sse_host,sse_host,sse_shared_host<S> > PX;
  typedef Ox<ON_HOST,B,sse_real,sse_host> OX;
  typedef RK43VisitorHost<B,S,S,real,PX,sse_real> Visitor;
  static const int N = block_size<S>::value;
  const int P = s.size();

  bind(s);

  #pragma omp parallel
  {
    typename temp_host_vector<sse_real>::type r1(N), r2(N), err(N), old(N);
    sse_real e, e2;
    real t, h, logfacold, logfac11, fac, e2max, e2s[BI_SSE_SIZE];
    int n, id, p;
    PX pax;
    OX x;

    #pragma omp for
    for (p = 0; p < P; p += BI_SSE_SIZE) {
      /* initialise shared memory from global memory */
      sse_shared_host_init<B,S>(p);

      t = t1;
      h = h_h0;
      logfacold = BI_MATH_LOG(BI_REAL(1.0e-4));
      n = 0;
      old = *sseSharedHostState;
      r1 = old;

      /* integrate */
      while (t < t2 && n < h_nsteps) {
        if (BI_REAL(0.1)*BI_MATH_FABS(h) <= BI_MATH_FABS(t)*h_uround) {
          // step size too small
        }
        if (t + BI_REAL(1.01)*h - t2 > BI_REAL(0.0)) {
          h = t2 - t;
          if (h <= BI_REAL(0.0)) {
            t = t2;
            break;
          }
        }

        /* stages */
        Visitor::stage1(t, h, p, pax, r1.buf(), r2.buf(), err.buf());
        *sseSharedHostState = r1;

        Visitor::stage2(t, h, p, pax, r1.buf(), r2.buf(), err.buf());
        *sseSharedHostState = r2;

        Visitor::stage3(t, h, p, pax, r1.buf(), r2.buf(), err.buf());
        *sseSharedHostState = r1;

        Visitor::stage4(t, h, p, pax, r1.buf(), r2.buf(), err.buf());
        *sseSharedHostState = r2;

        Visitor::stage5(t, h, p, pax, r1.buf(), r2.buf(), err.buf());
        *sseSharedHostState = r1;

        /* determine largest error among trajectories */
        e2 = BI_REAL(0.0);
        for (id = 0; id < N; ++id) {
          e = err(id)*h/(sse_max(sse_fabs(old(id)), sse_fabs(r1(id)))*h_rtoler + h_atoler);
          e2 += e*e;
        }
        e2.store(e2s);
        e2max = e2s[0];
        for (id = 1; id < BI_SSE_SIZE; ++id) {
          if (e2s[id] > e2max) {
            e2max = e2s[id];
          }
        }
        e2max /= N;

        if (e2max <= BI_REAL(1.0)) {
          /* accept */
          t += h;
          if (t < t2) {
            old = r1;
          }
        } else {
          /* reject */
          r1 = old;
          *sseSharedHostState = old;
        }

        /* compute next step size */
        if (t < t2) {
          logfac11 = h_expo*BI_MATH_LOG(e2max);
          if (e2max > BI_REAL(1.0)) {
            /* step was rejected */
            h *= BI_MATH_MAX(h_facl, BI_MATH_EXP(h_logsafe - logfac11));
          } else {
            /* step was accepted */
            fac = BI_MATH_EXP(h_beta*logfacold + h_logsafe - logfac11); // Lund-stabilization
            fac = BI_MATH_MIN(h_facr, BI_MATH_MAX(h_facl, fac)); // bound
            h *= fac;
            logfacold = BI_REAL(0.5)*BI_MATH_LOG(BI_MATH_MAX(e2max, BI_REAL(1.0e-8)));
          }
        }

        ++n;
      }

      /* write from shared back to global memory */
      sse_shared_host_commit<B,S>(p);
    }
  }

  unbind(s);
}

#endif