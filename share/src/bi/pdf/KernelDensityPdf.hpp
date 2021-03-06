/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 *
 * Imported from dysii 1.4.0, originally indii/ml/aux/KernelDensityPdf.hpp
 */
#ifndef BI_PDF_KERNELDENSITYPDF_HPP
#define BI_PDF_KERNELDENSITYPDF_HPP

#include "../kd/kde.hpp"
#include "../kd/FastGaussianKernel.hpp"
#include "../math/view.hpp"
#include "../math/operation.hpp"
#include "../random/Random.hpp"

#include "boost/serialization/split_member.hpp"

namespace bi {
/**
 * Kernel density estimate as pdf.
 *
 * @ingroup math_pdf
 *
 * @tparam V1 Vector type.
 * @tparam M1 Matrix type.
 * @tparam S1 Partitioner type.
 * @tparam K1 Kernel type.
 *
 * @section KernelDensityPdf_Serialization Serialization
 *
 * This class supports serialization through the Boost.Serialization
 * library.
 *
 * @section Concepts
 *
 * #concept::Pdf
 *
 * @todo Review for log-variable support
 * @todo Review for arbitrary kernel support (assumes Gaussian in many cases)
 */
template<class V1 = host_vector<real>, class M1 = host_matrix<real>,
    class S1 = MedianPartitioner, class K1 = FastGaussianKernel>
class KernelDensityPdf : private ExpGaussianPdf<V1,M1> {
public:
  /**
   * Constructor.
   *
   * @param X Samples.
   * @param lw Log-weights.
   * @param K Kernel.
   * @param logs Indices of log-variables.
   */
  KernelDensityPdf(const M1 X, const V1 lw, const K1& K,
      const std::set<int>& logs);

  /**
   * Constructor.
   *
   * @param X Samples.
   * @param lw Log-weights.
   * @param K Kernel.
   */
  KernelDensityPdf(const M1 X, const V1 lw, const K1& K);

  /**
   * Copy constructor.
   */
  KernelDensityPdf(const KernelDensityPdf<V1,M1,S1,K1>& o);

  /**
   * Destructor.
   */
  ~KernelDensityPdf();

  /**
   * Assignment operator. Both sides must have the same dimensionality.
   */
  KernelDensityPdf<V1,M1,S1,K1>& operator=(const KernelDensityPdf<V1,M1,S1,K1>& o);

  using ExpGaussianPdf<V1,M1>::size;
  using ExpGaussianPdf<V1,M1>::getLogs;
  using ExpGaussianPdf<V1,M1>::setLogs;
  using ExpGaussianPdf<V1,M1>::addLogs;
  using ExpGaussianPdf<V1,M1>::addLog;
  using ExpGaussianPdf<V1,M1>::mean;
  using ExpGaussianPdf<V1,M1>::cov;
  using ExpGaussianPdf<V1,M1>::std;
  using ExpGaussianPdf<V1,M1>::prec;

  /**
   * @copydoc concept::Pdf::sample()
   */
  template<class V2>
  void sample(Random& rng, V2 x);

  /**
   * @copydoc concept::Pdf::samples()
   */
  template<class M2>
  void samples(Random& rng, M2 X);

  /**
   * @copydoc concept::Pdf::density()
   */
  template<class V2>
  real density(const V2 x);

  /**
   * @copydoc concept::Pdf::densities()
   */
  template<class M2, class V2>
  void densities(const M2 X, V2 p, const bool clear = false);

  /**
   * @copydoc concept::Pdf::logDensity()
   */
  template<class V2>
  real logDensity(const V2 x);

  /**
   * @copydoc concept::Pdf::logDensities()
   */
  template<class M2, class V2>
  void logDensities(const M2 X, V2 p, const bool clear = false);

  /**
   * @copydoc concept::Pdf::operator()(const V1)
   */
  template<class V2>
  real operator()(const V2 x);

protected:
  /**
   * Scalar type.
   */
  typedef typename V1::value_type value_type;

  /**
   * Kd tree over samples.
   */
  KDTree<V1>* tree;

  /**
   * Samples.
   */
  M1 X;

  /**
   * Weights.
   */
  V1 lw;

  /**
   * Kernel.
   */
  K1 K;

  /**
   * Total weight.
   */
  real W;

  /**
   * Perform precalculations.
   */
  void init();

private:
  /**
   * Serialize.
   */
  template<class Archive>
  void save(Archive& ar, const unsigned version) const;

  /**
   * Restore from serialization.
   */
  template<class Archive>
  void load(Archive& ar, const unsigned version);

  /*
   * Boost.Serialization requirements.
   */
  BOOST_SERIALIZATION_SPLIT_MEMBER()
  friend class boost::serialization::access;
};
}

#include "misc.hpp"
#include "functor.hpp"
#include "../math/pi.hpp"
#include "../math/view.hpp"
#include "../math/sim_temp_vector.hpp"
#include "../misc/assert.hpp"

#include "boost/serialization/base_object.hpp"
#include "boost/typeof/typeof.hpp"

template<class V1, class M1, class S1, class K1>
bi::KernelDensityPdf<V1,M1,S1,K1>::KernelDensityPdf(const M1 X, const V1 lw,
    const K1& K, const std::set<int>& logs) :
    ExpGaussianPdf<V1,M1>(X.size2(), logs), X(X.size1(), X.size2()),
    lw(lw.size()), K(K) {
  this->X = X;
  this->lw = lw;
  init();
}

template<class V1, class M1, class S1, class K1>
bi::KernelDensityPdf<V1,M1,S1,K1>::KernelDensityPdf(const M1 X, const V1 lw,
    const K1& K) : ExpGaussianPdf<V1,M1>(X.size2()), X(X.size1(), X.size2()),
    lw(lw.size()), K(K) {
  this->X = X;
  this->lw = lw;
  init();
}

template<class V1, class M1, class S1, class K1>
bi::KernelDensityPdf<V1,M1,S1,K1>::KernelDensityPdf(
    const KernelDensityPdf<V1,M1,S1,K1>& o) : ExpGaussianPdf<V1,M1>(o),
    X(o.X.size1(), o.X.size2()), lw(o.lw.size()), K(o.K), W(o.W) {
  X = o.X;
  lw = o.lw;
}

template<class V1, class M1, class S1, class K1>
bi::KernelDensityPdf<V1,M1,S1,K1>::~KernelDensityPdf() {
  delete tree;
}

template<class V1, class M1, class S1, class K1>
bi::KernelDensityPdf<V1,M1,S1,K1>&
    bi::KernelDensityPdf<V1,M1,S1,K1>::operator=(
    const KernelDensityPdf<V1,M1,S1,K1>& o) {
  ExpGaussianPdf<V1,M1>::operator=(o);
  X.resize(o.X.size1(), o.X.size2(), false);
  lw.resize(o.lw.size(), false);
  X = o.X;
  lw = o.lw;
  K = o.K;
  W = o.W;

  return *this;
}

template<class V1, class M1, class S1, class K1>
template<class V2>
inline void bi::KernelDensityPdf<V1,M1,S1,K1>::sample(Random& rng, V2 x) {
  /* pre-condition */
  BI_ASSERT(x.size() == size());

  int p = rng.multinomial(lw);
  rng.gaussians(x);
  scal(K.bandwidth(), x);
  axpy(1.0, row(this->X, p), x);
  trmv(1.0, std(), x, 'R', 'U');
  axpy(1.0, mean(), x);
  exp_vector(x, this->logs);
}

template<class V1, class M1, class S1, class K1>
template<class M2>
void bi::KernelDensityPdf<V1,M1,S1,K1>::samples(Random& rng, M2 X) {
  /* pre-condition */
  BI_ASSERT(X.size2() == size());

  temp_host_vector<real>::type ps(X.size1());
  int i;

  rng.gaussians(vec(X));
  matrix_scal(K.bandwidth(), X);
  rng.multinomials(lw, ps);
  for (i = 0; i < X.size1(); ++i) {
    axpy(1.0, row(this->X, ps(i)), row(X, i));
  }
  trmm(1.0, std(), X, 'R', 'U');
  add_rows(X, mean());
  exp_columns(X, this->logs);
}

template<class V1, class M1, class S1, class K1>
template<class V2>
real bi::KernelDensityPdf<V1,M1,S1,K1>::density(const V2 x) {
  if (tree->getRoot() == NULL) {
    return 0.0;
  }

  typename sim_temp_vector<V2>::type z(x.size()), d(x.size());
  KDTreeNode<V1,M1>* node = tree->getRoot();
  std::stack<KDTreeNode<V1,M1>*> nodes;
  double p = 0.0;
  int i;

  /* standardise input if necessary */
  z = x;
  standardise(*this, vector_as_row_matrix(z));

  /* traverse tree */
  nodes.push(node);
  while (!nodes.empty()) {
    node = nodes.top();
    nodes.pop();

    if (node->isLeaf()) {
      i = node->getIndex();
      d = row(X, i);
      axpy(-1.0, d, x);
      p += bi::exp(lw(i) + K.logDensity(d));
    } else if (node->isPrune()) {
      std::vector<int>& is = node->getIndices();
      for (i = 0; i < is.size(); i++) {
        d = row(X, is[i]);
        axpy(-1.0, x, d);
        p += bi::exp(lw(is[i]) + K.logDensity(d));
      }
    } else {
      /* should we recurse? */
      node->difference(x, d);
      if (K(d) > 0.0) {
        nodes.push(node->getLeft());
        nodes.push(node->getRight());
      }
    }
  }
  p *= this->ZI/W;

  return p;
}

template<class V1, class M1, class S1, class K1>
template<class M2, class V2>
void bi::KernelDensityPdf<V1,M1,S1,K1>::densities(const M2 X, V2 p) {
  temp_host_matrix<real>::type Z(X.size1(), X.size2());
  Z = X;
  standardise(*this, Z);
  KDTree<V1,M1> queryTree(Z, S1());
  dualTreeDensity(queryTree, *this->tree, K, p, clear);
  scal(this->invZ/W, p);
}

template<class V1, class M1, class S1, class K1>
template<class V2>
real bi::KernelDensityPdf<V1,M1,S1,K1>::logDensity(const V2 x) {
  return bi::log(density(x));
}

template<class V1, class M1, class S1, class K1>
template<class M2, class V2>
void bi::KernelDensityPdf<V1,M1,S1,K1>::logDensities(const M2 X, V2 p,
    const bool clear) {
  if (clear) {
    densities(X, p, clear);
    log_elements(p, p);
  } else {
    typename sim_temp_vector<V2>::type p1(p.size());
    densities(X, p1, true);
    log_elements(p1, p1);
    add_elements(p, p1, p);
  }
}

template<class V1, class M1, class S1, class K1>
template<class V2>
real bi::KernelDensityPdf<V1,M1,S1,K1>::operator()(const V2 x) {
  return density(x);
}

template<class V1, class M1, class S1, class K1>
void bi::KernelDensityPdf<V1,M1,S1,K1>::init() {
  typename sim_temp_vector<V1>::type w(lw.size());

  /* renormalise log-weights */
  value_type mx = expu_elements(lw, w);

  /* sum weights */
  W = sum_reduce(w);

  /* compute mean and covariance */
  bi::mean(X, w, ExpGaussianPdf<V1,M1>::mean());
  bi::cov(X, w, ExpGaussianPdf<V1,M1>::mean(), ExpGaussianPdf<V1,M1>::cov());
  ExpGaussianPdf<V1,M1>::init();

  /* standardise samples */
  standardise(*this, X);

  /* shrink samples */
  real a = std::sqrt(1.0 - std::pow(K.bandwidth(), 2));
  matrix_scal(a, X);

  /* build kd tree */
  tree = new KDTree<V1,M1>(X, lw, S1());
}

template<class V1, class M1, class S1, class K1>
template<class Archive>
void bi::KernelDensityPdf<V1,M1,S1,K1>::save(Archive& ar, const unsigned version) const {
  ar & X;
  ar & lw;
  ar & K;
  ar * W;
}

template<class V1, class M1, class S1, class K1>
template<class Archive>
void bi::KernelDensityPdf<V1,M1,S1,K1>::load(Archive& ar, const unsigned version) {
  ar & X;
  ar & lw;
  ar & K;
  ar * W;
}

#endif
