/**
 * @todo Allow to specify a minimum number of blocks.
 *
 * @todo * (L) Fix ESS threshold for MPI.
 *
 * @todo * (A) Make ESS threshold clear with adaptive particle filter.
 *
 * @todo Adapting number of samples of initial conditions.
 *
 * @todo Adapting the number of theta-particles in SMC^2.
 *
 * @todo Include meta-data on command in output NetCDF files, include *.bi
 * file also?
 *
 * @todo Output diagnostics (e.g. number of particles at each time for each
 * filter run during PMMH).
 *
 * @todo * (L) Custom block/action. Bi C++ interface for writing custom functions
 * in Bi files.
 *
 * @todo * (All) Example models for release. Look at POMP?
 *
 * @todo Test suite (list of bi commands that can be run to test
 * changes). Regression tests.
 *
 * @todo * (P) Output trajectories with SMC^2.
 *
 * @todo Swap GPU memory into host memory, swap host memory to disk.
 *
 * @todo * (P) Documentation on producing NetCDF files from e.g. CSV files.
 *
 * @todo Fixed lag smoother.
 *
 * @todo Collapsed state-space model.
 *
 * @todo Log and logit transforms of parameters (e.g. --transform-*).
 * 
 * @todo Evidence estimation using PMCMC.
 *
 * @todo Univariate proposals.
 *
 * @todo Test functions for time T.
 *
 * @todo * (L) Multivariate normal
 *
 * @todo * (P) Exponential pdf, poisson pdf.
 *
 * @todo PMMH normalising constant estimates at each time.
 *
 * @todo * (L) Review OctBi.
 *
 * @todo * (A) Get OctBi working with MATLAB.
 *
 * @todo * (P) Review RBi.
 *
 * @todo Use new NetCDF C++ interface.
 *
 * @todo Add --cuda-arch flag to set sm_13, sm_20 or sm_30 etc.
 *
 * @todo Need a locking system to ensure exclusive use of build directories.
 * Having a problem on cluster where multiple jobs are trying to build
 * simultaneously in the same directory and conflict with each other. Could,
 * for example, put a lock file in each directory to reserve it for a
 * particular process, then delete the file when that process is done with it.
 * If another process wants to use it, rather than waiting, it could simply
 * create a new directory with another name and work from there.
 *
 * @todo Tidy up output file variable names, perhaps precede internal
 * variables with the program or schema name, e.g. simulate.time,
 * filter.logweight etc.
 *
 * @todo bi smooth
 *
 * @todo AVX support (should be very similar to SSE support, but with slightly
 * different intrinsics).
 *
 * @todo Replace BI_REAL(1.0/0.0) with BI_INF constant. Introduce named
 * literals such as true, false, pi, nan, inf into language.
 */
