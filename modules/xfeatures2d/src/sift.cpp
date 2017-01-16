/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/**********************************************************************************************\
 Implementation of SIFT is based on the code from http://blogs.oregonstate.edu/hess/code/sift/
 Below is the original copyright.

//    Copyright (c) 2006-2010, Rob Hess <hess@eecs.oregonstate.edu>
//    All rights reserved.

//    The following patent has been issued for methods embodied in this
//    software: "Method and apparatus for identifying scale invariant features
//    in an image and use of same for locating an object in an image," David
//    G. Lowe, US Patent 6,711,293 (March 23, 2004). Provisional application
//    filed March 8, 1999. Asignee: The University of British Columbia. For
//    further details, contact David Lowe (lowe@cs.ubc.ca) or the
//    University-Industry Liaison Office of the University of British
//    Columbia.

//    Note that restrictions imposed by this patent (and possibly others)
//    exist independently of and may be in conflict with the freedoms granted
//    in this license, which refers to copyright of the program, not patents
//    for any methods that it implements.  Both copyright and patent law must
//    be obeyed to legally use and redistribute this program and it is not the
//    purpose of this license to induce you to infringe any patents or other
//    property right claims or to contest validity of any such claims.  If you
//    redistribute or use the program, then this license merely protects you
//    from committing copyright infringement.  It does not protect you from
//    committing patent infringement.  So, before you do anything with this
//    program, make sure that you have permission to do so not merely in terms
//    of copyright, but also in terms of patent law.

//    Please note that this license is not to be understood as a guarantee
//    either.  If you use the program according to this license, but in
//    conflict with patent law, it does not mean that the licensor will refund
//    you for any losses that you incur if you are sued for your patent
//    infringement.

//    Redistribution and use in source and binary forms, with or without
//    modification, are permitted provided that the following conditions are
//    met:
//        * Redistributions of source code must retain the above copyright and
//          patent notices, this list of conditions and the following
//          disclaimer.
//        * Redistributions in binary form must reproduce the above copyright
//          notice, this list of conditions and the following disclaimer in
//          the documentation and/or other materials provided with the
//          distribution.
//        * Neither the name of Oregon State University nor the names of its
//          contributors may be used to endorse or promote products derived
//          from this software without specific prior written permission.

//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
//    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//    HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**********************************************************************************************/

#include "precomp.hpp"
#include <iostream>
#include <stdarg.h>
#include <opencv2/core/hal/hal.hpp>

namespace cv
{
namespace xfeatures2d
{

/*!
 SIFT implementation.

 The class implements SIFT algorithm by D. Lowe.
 */
class SIFT_Impl : public SIFT
{
public:
    explicit SIFT_Impl( int nfeatures = 0, int nOctaveLayers = 3,
                          double contrastThreshold = 0.04, double edgeThreshold = 10,
                          double sigma = 1.6, bool useNegativeOctave = true );

    //! returns the descriptor size in floats (128)
    int descriptorSize() const;

    //! returns the descriptor type
    int descriptorType() const;

    //! returns the default norm type
    int defaultNorm() const;

    //! finds the keypoints and computes descriptors for them using SIFT algorithm.
    //! Optionally it can compute descriptors for the user-provided keypoints
    void detectAndCompute(InputArray img, InputArray mask,
                    std::vector<KeyPoint>& keypoints,
                    OutputArray descriptors,
                    bool useProvidedKeypoints = false);

    void buildGaussianPyramid( const Mat& base, std::vector<Mat>& pyr, int nOctaves ) const;
    void buildDoGPyramid( const std::vector<Mat>& pyr, std::vector<Mat>& dogpyr ) const;
    void findScaleSpaceExtrema( const std::vector<Mat>& gauss_pyr, const std::vector<Mat>& dog_pyr,
                               std::vector<KeyPoint>& keypoints ) const;

protected:
    CV_PROP_RW int nfeatures;
    CV_PROP_RW int nOctaveLayers;
    CV_PROP_RW double contrastThreshold;
    CV_PROP_RW double edgeThreshold;
    CV_PROP_RW double sigma;
    CV_PROP_RW bool useNegativeOctave;
};

Ptr<SIFT> SIFT::create( int _nfeatures, int _nOctaveLayers,
                        double _contrastThreshold, double _edgeThreshold, double _sigma,
                        bool _useNegativeOctave)
{
    return makePtr<SIFT_Impl>(_nfeatures, _nOctaveLayers, _contrastThreshold, _edgeThreshold, _sigma, _useNegativeOctave);
}

/******************************* Defs and macros *****************************/

// default width of descriptor histogram array
static const int SIFT_DESCR_WIDTH = 4;

// default number of bins per histogram in descriptor array
static const int SIFT_DESCR_HIST_BINS = 8;

// assumed gaussian blur for input image
static const float SIFT_INIT_SIGMA = 0.5f;

// width of border in which to ignore keypoints
static const int SIFT_IMG_BORDER = 5;

// maximum steps of keypoint interpolation before failure
static const int SIFT_MAX_INTERP_STEPS = 5;

// default number of bins in histogram for orientation assignment
static const int SIFT_ORI_HIST_BINS = 36;

// determines gaussian sigma for orientation assignment
static const float SIFT_ORI_SIG_FCTR = 1.5f;

// determines the radius of the region used in orientation assignment
static const float SIFT_ORI_RADIUS = 3 * SIFT_ORI_SIG_FCTR;

// orientation magnitude relative to max that results in new feature
static const float SIFT_ORI_PEAK_RATIO = 0.8f;

// determines the size of a single descriptor orientation histogram
static const float SIFT_DESCR_SCL_FCTR = 3.f;

// threshold on magnitude of elements of descriptor vector
static const float SIFT_DESCR_MAG_THR = 0.2f;

// factor used to convert floating-point descriptor to unsigned char
static const float SIFT_INT_DESCR_FCTR = 512.f;

#if 0
// intermediate type used for DoG pyramids
typedef short sift_wt;
static const int SIFT_FIXPT_SCALE = 48;
#else
// intermediate type used for DoG pyramids
typedef float sift_wt;
static const int SIFT_FIXPT_SCALE = 1;
#endif

static inline void
unpackOctave(const KeyPoint& kpt, int& octave, int& layer, float& scale)
{
    octave = kpt.octave & 255;
    layer = (kpt.octave >> 8) & 255;
    octave = octave < 128 ? octave : (-128 | octave);
    scale = octave >= 0 ? 1.f/(1 << octave) : (float)(1 << -octave);
}

static Mat createInitialImage( const Mat& img, bool doubleImageSize, float sigma )
{
    Mat gray, gray_fpt;
    if( img.channels() == 3 || img.channels() == 4 )
        cvtColor(img, gray, COLOR_BGR2GRAY);
    else
        img.copyTo(gray);
    gray.convertTo(gray_fpt, DataType<sift_wt>::type, SIFT_FIXPT_SCALE, 0);

    float sig_diff;

    if( doubleImageSize )
    {
        sig_diff = sqrtf( std::max(sigma * sigma - SIFT_INIT_SIGMA * SIFT_INIT_SIGMA * 4, 0.01f) );
        Mat dbl, tmp;
        resize(gray_fpt, dbl, Size(gray.cols*2, gray.rows*2), 0, 0, INTER_LINEAR);
        GaussianBlur(dbl, tmp, Size(), sig_diff, sig_diff);
        return tmp;
    }
    else
    {
        Mat tmp;
        sig_diff = sqrtf( std::max(sigma * sigma - SIFT_INIT_SIGMA * SIFT_INIT_SIGMA, 0.01f) );
        GaussianBlur(gray_fpt, tmp, Size(), sig_diff, sig_diff);
        return tmp;
    }
}


void SIFT_Impl::buildGaussianPyramid( const Mat& base, std::vector<Mat>& pyr, int nOctaves ) const
{
    std::vector<double> sig(nOctaveLayers + 3);
    pyr.resize(nOctaves*(nOctaveLayers + 3));

    // precompute Gaussian sigmas using the following formula:
    //  \sigma_{total}^2 = \sigma_{i}^2 + \sigma_{i-1}^2
    sig[0] = sigma;
    double k = std::pow( 2., 1. / nOctaveLayers );
    for( int i = 1; i < nOctaveLayers + 3; i++ )
    {
        double sig_prev = std::pow(k, (double)(i-1))*sigma;
        double sig_total = sig_prev*k;
        sig[i] = std::sqrt(sig_total*sig_total - sig_prev*sig_prev);
    }

    for( int o = 0; o < nOctaves; o++ )
    {
        for( int i = 0; i < nOctaveLayers + 3; i++ )
        {
            Mat& dst = pyr[o*(nOctaveLayers + 3) + i];
            if( o == 0  &&  i == 0 )
                dst = base;
            // base of new octave is halved image from end of previous octave
            else if( i == 0 )
            {
                const Mat& src = pyr[(o-1)*(nOctaveLayers + 3) + nOctaveLayers];
                resize(src, dst, Size(src.cols/2, src.rows/2),
                       0, 0, INTER_NEAREST);
            }
            else
            {
                const Mat& src = pyr[o*(nOctaveLayers + 3) + i-1];
                GaussianBlur(src, dst, Size(), sig[i], sig[i]);
            }
        }
    }
}

class buildDoGPyramidComputer : public ParallelLoopBody
{
public:
    buildDoGPyramidComputer(
        int _nOctaves,
        int _nOctaveLayers,
        const std::vector<Mat>& _gpyr,
        std::vector<Mat>& _dogpyr)
        : nOctaves(_nOctaves),
          nOctaveLayers(_nOctaveLayers),
          gpyr(_gpyr),
          dogpyr(_dogpyr) { }

    void operator()( const cv::Range& range ) const
    {
        const int begin = range.start;
        const int end = range.end;

        for( int a = begin; a < end; a++ )
        {
            int o = a / (nOctaveLayers + 2);
            int i = a % (nOctaveLayers + 2);

            const Mat& src1 = gpyr[o*(nOctaveLayers + 3) + i];
            const Mat& src2 = gpyr[o*(nOctaveLayers + 3) + i + 1];
            Mat& dst = dogpyr[o*(nOctaveLayers + 2) + i];
            subtract(src2, src1, dst, noArray(), DataType<sift_wt>::type);
        }
    }

private:
    int nOctaves;
    int nOctaveLayers;
    const std::vector<Mat>& gpyr;
    std::vector<Mat>& dogpyr;
};


void SIFT_Impl::buildDoGPyramid( const std::vector<Mat>& gpyr, std::vector<Mat>& dogpyr ) const
{
    int nOctaves = (int)gpyr.size()/(nOctaveLayers + 3);
    dogpyr.resize( nOctaves*(nOctaveLayers + 2) );


    parallel_for_(Range(0, nOctaves * (nOctaveLayers + 2)), buildDoGPyramidComputer(nOctaves, nOctaveLayers, gpyr, dogpyr));

    /*
    for( int o = 0; o < nOctaves; o++ )
    {
        for( int i = 0; i < nOctaveLayers + 2; i++ )
        {
            const Mat& src1 = gpyr[o*(nOctaveLayers + 3) + i];
            const Mat& src2 = gpyr[o*(nOctaveLayers + 3) + i + 1];
            Mat& dst = dogpyr[o*(nOctaveLayers + 2) + i];
            subtract(src2, src1, dst, noArray(), DataType<sift_wt>::type);
        }
    }*/
}


// Computes a gradient orientation histogram at a specified pixel
static float calcOrientationHist( const Mat& img, Point pt, int radius,
                                  float sigma, float* hist, int n )
{
    int i, j, k, len = (radius*2+1)*(radius*2+1);

    float expf_scale = -1.f/(2.f * sigma * sigma);
    AutoBuffer<float> buf(len*4 + n+4);
    float *X = buf, *Y = X + len, *Mag = X, *Ori = Y + len, *W = Ori + len;
    float* temphist = W + len + 2;

    std::memset(temphist, 0, n * sizeof(float));

    for( i = -radius, k = 0; i <= radius; i++ )
    {
        int y = pt.y + i;
        if( y <= 0 || y >= img.rows - 1 )
            continue;
        for( j = -radius; j <= radius; j++ )
        {
            int x = pt.x + j;
            if( x <= 0 || x >= img.cols - 1 )
                continue;

            float dx = (float)(img.at<sift_wt>(y, x+1) - img.at<sift_wt>(y, x-1));
            float dy = (float)(img.at<sift_wt>(y-1, x) - img.at<sift_wt>(y+1, x));

            X[k] = dx; Y[k] = dy; W[k] = (i*i + j*j)*expf_scale;
            k++;
        }
    }

    len = k;

    // compute gradient values, orientations and the weights over the pixel neighborhood
    cv::hal::exp32f(W, W, len);
    cv::hal::fastAtan2(Y, X, Ori, len, true);
    cv::hal::magnitude32f(X, Y, Mag, len);

    k = 0;
#if CV_AVX2
    __m256 __nd360 = _mm256_set1_ps(n/360.f);
    __m256i __n = _mm256_set1_epi32(n);
    int CV_DECL_ALIGNED(32) bin_buf[8];
    float CV_DECL_ALIGNED(32) w_mul_mag_buf[8];
    for ( ; k <= len - 8; k+=8 )
    {
        __m256i __bin = _mm256_cvtps_epi32(_mm256_round_ps(_mm256_mul_ps(__nd360, _mm256_loadu_ps(&Ori[k])), _MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));

        __bin = _mm256_sub_epi32(__bin, _mm256_and_si256(
            __n,
            _mm256_or_si256(
                _mm256_cmpeq_epi32(__bin, __n),
                _mm256_cmpgt_epi32(__bin, __n))));
        __bin = _mm256_add_epi32(__bin, _mm256_and_si256(
            __n,
            _mm256_cmpgt_epi32(_mm256_setzero_si256(), __bin)));

        __m256 __w_mul_mag = _mm256_mul_ps(_mm256_loadu_ps(&W[k]), _mm256_loadu_ps(&Mag[k]));

        _mm256_store_si256((__m256i *) bin_buf, __bin);
        _mm256_store_ps(w_mul_mag_buf, __w_mul_mag);

        temphist[bin_buf[0]] += w_mul_mag_buf[0];
        temphist[bin_buf[1]] += w_mul_mag_buf[1];
        temphist[bin_buf[2]] += w_mul_mag_buf[2];
        temphist[bin_buf[3]] += w_mul_mag_buf[3];
        temphist[bin_buf[4]] += w_mul_mag_buf[4];
        temphist[bin_buf[5]] += w_mul_mag_buf[5];
        temphist[bin_buf[6]] += w_mul_mag_buf[6];
        temphist[bin_buf[7]] += w_mul_mag_buf[7];
    }
#endif
    for( ; k < len; k++ )
    {
        int bin = cvRound((n/360.f)*Ori[k]);
        if( bin >= n )
            bin -= n;
        if( bin < 0 )
            bin += n;
        temphist[bin] += W[k]*Mag[k];
    }

    // smooth the histogram
    temphist[-1] = temphist[n-1];
    temphist[-2] = temphist[n-2];
    temphist[n] = temphist[0];
    temphist[n+1] = temphist[1];

    i = 0;
#if CV_AVX2
    __m256 __d_1_16 = _mm256_set1_ps(1.f/16.f);
    __m256 __d_4_16 = _mm256_set1_ps(4.f/16.f);
    __m256 __d_6_16 = _mm256_set1_ps(6.f/16.f);
    for( ; i <= n - 8; i+=8 )
    {
#if CV_FMA3
        __m256 __hist =
            _mm256_fmadd_ps(
                _mm256_add_ps(
                    _mm256_loadu_ps(&temphist[i-2]),
                    _mm256_loadu_ps(&temphist[i+2])),
                __d_1_16,
                _mm256_fmadd_ps(
                    _mm256_add_ps(
                        _mm256_loadu_ps(&temphist[i-1]),
                        _mm256_loadu_ps(&temphist[i+1])),
                    __d_4_16,
                    _mm256_mul_ps(
                        _mm256_loadu_ps(&temphist[i]),
                        __d_6_16)));
#else
        __m256 __hist =
            _mm256_add_ps(
                _mm256_mul_ps(
                    _mm256_add_ps(
                        _mm256_loadu_ps(&temphist[i-2]),
                        _mm256_loadu_ps(&temphist[i+2])),
                    __d_1_16),
                _mm256_add_ps(
                    _mm256_mul_ps(
                        _mm256_add_ps(
                            _mm256_loadu_ps(&temphist[i-1]),
                            _mm256_loadu_ps(&temphist[i+1])),
                        __d_4_16),
                    _mm256_mul_ps(
                        _mm256_loadu_ps(&temphist[i]),
                        __d_6_16)));
#endif
        _mm256_storeu_ps(&hist[i], __hist);
    }
#endif
    for( ; i < n; i++ )
    {
        hist[i] = (temphist[i-2] + temphist[i+2])*(1.f/16.f) +
            (temphist[i-1] + temphist[i+1])*(4.f/16.f) +
            temphist[i]*(6.f/16.f);
    }

    float maxval = hist[0];
    for( i = 1; i < n; i++ )
        maxval = std::max(maxval, hist[i]);

    return maxval;
}


//
// Interpolates a scale-space extremum's location and scale to subpixel
// accuracy to form an image feature. Rejects features with low contrast.
// Based on Section 4 of Lowe's paper.
static bool adjustLocalExtrema( const std::vector<Mat>& dog_pyr, KeyPoint& kpt, int octv,
                                int& layer, int& r, int& c, int nOctaveLayers,
                                float contrastThreshold, float edgeThreshold, float sigma )
{
    const float img_scale = 1.f/(255*SIFT_FIXPT_SCALE);
    const float deriv_scale = img_scale*0.5f;
    const float second_deriv_scale = img_scale;
    const float cross_deriv_scale = img_scale*0.25f;

    float xi=0, xr=0, xc=0, contr=0;
    int i = 0;

    for( ; i < SIFT_MAX_INTERP_STEPS; i++ )
    {
        int idx = octv*(nOctaveLayers+2) + layer;
        const Mat& img = dog_pyr[idx];
        const Mat& prev = dog_pyr[idx-1];
        const Mat& next = dog_pyr[idx+1];

        Vec3f dD((img.at<sift_wt>(r, c+1) - img.at<sift_wt>(r, c-1))*deriv_scale,
                 (img.at<sift_wt>(r+1, c) - img.at<sift_wt>(r-1, c))*deriv_scale,
                 (next.at<sift_wt>(r, c) - prev.at<sift_wt>(r, c))*deriv_scale);

        float v2 = (float)img.at<sift_wt>(r, c)*2;
        float dxx = (img.at<sift_wt>(r, c+1) + img.at<sift_wt>(r, c-1) - v2)*second_deriv_scale;
        float dyy = (img.at<sift_wt>(r+1, c) + img.at<sift_wt>(r-1, c) - v2)*second_deriv_scale;
        float dss = (next.at<sift_wt>(r, c) + prev.at<sift_wt>(r, c) - v2)*second_deriv_scale;
        float dxy = (img.at<sift_wt>(r+1, c+1) - img.at<sift_wt>(r+1, c-1) -
                     img.at<sift_wt>(r-1, c+1) + img.at<sift_wt>(r-1, c-1))*cross_deriv_scale;
        float dxs = (next.at<sift_wt>(r, c+1) - next.at<sift_wt>(r, c-1) -
                     prev.at<sift_wt>(r, c+1) + prev.at<sift_wt>(r, c-1))*cross_deriv_scale;
        float dys = (next.at<sift_wt>(r+1, c) - next.at<sift_wt>(r-1, c) -
                     prev.at<sift_wt>(r+1, c) + prev.at<sift_wt>(r-1, c))*cross_deriv_scale;

        Matx33f H(dxx, dxy, dxs,
                  dxy, dyy, dys,
                  dxs, dys, dss);

        Vec3f X = H.solve(dD, DECOMP_LU);

        xi = -X[2];
        xr = -X[1];
        xc = -X[0];

        if( std::abs(xi) < 0.5f && std::abs(xr) < 0.5f && std::abs(xc) < 0.5f )
            break;

        if( std::abs(xi) > (float)(INT_MAX/3) ||
            std::abs(xr) > (float)(INT_MAX/3) ||
            std::abs(xc) > (float)(INT_MAX/3) )
            return false;

        c += cvRound(xc);
        r += cvRound(xr);
        layer += cvRound(xi);

        if( layer < 1 || layer > nOctaveLayers ||
            c < SIFT_IMG_BORDER || c >= img.cols - SIFT_IMG_BORDER  ||
            r < SIFT_IMG_BORDER || r >= img.rows - SIFT_IMG_BORDER )
            return false;
    }

    // ensure convergence of interpolation
    if( i >= SIFT_MAX_INTERP_STEPS )
        return false;

    {
        int idx = octv*(nOctaveLayers+2) + layer;
        const Mat& img = dog_pyr[idx];
        const Mat& prev = dog_pyr[idx-1];
        const Mat& next = dog_pyr[idx+1];
        Matx31f dD((img.at<sift_wt>(r, c+1) - img.at<sift_wt>(r, c-1))*deriv_scale,
                   (img.at<sift_wt>(r+1, c) - img.at<sift_wt>(r-1, c))*deriv_scale,
                   (next.at<sift_wt>(r, c) - prev.at<sift_wt>(r, c))*deriv_scale);
        float t = dD.dot(Matx31f(xc, xr, xi));

        contr = img.at<sift_wt>(r, c)*img_scale + t * 0.5f;
        if( std::abs( contr ) * nOctaveLayers < contrastThreshold )
            return false;

        // principal curvatures are computed using the trace and det of Hessian
        float v2 = img.at<sift_wt>(r, c)*2.f;
        float dxx = (img.at<sift_wt>(r, c+1) + img.at<sift_wt>(r, c-1) - v2)*second_deriv_scale;
        float dyy = (img.at<sift_wt>(r+1, c) + img.at<sift_wt>(r-1, c) - v2)*second_deriv_scale;
        float dxy = (img.at<sift_wt>(r+1, c+1) - img.at<sift_wt>(r+1, c-1) -
                     img.at<sift_wt>(r-1, c+1) + img.at<sift_wt>(r-1, c-1)) * cross_deriv_scale;
        float tr = dxx + dyy;
        float det = dxx * dyy - dxy * dxy;

        if( det <= 0 || tr*tr*edgeThreshold >= (edgeThreshold + 1)*(edgeThreshold + 1)*det )
            return false;
    }

    kpt.pt.x = (c + xc) * (1 << octv);
    kpt.pt.y = (r + xr) * (1 << octv);
    kpt.octave = octv + (layer << 8) + (cvRound((xi + 0.5)*255) << 16);
    kpt.size = sigma*powf(2.f, (layer + xi) / nOctaveLayers)*(1 << octv)*2;
    kpt.response = std::abs(contr);

    return true;
}

//typedef tbb::enumerable_thread_specific< std::vector<KeyPoint> > KeyPointsTLSType;
//KeyPointsTLSType KeyPointsTLS;

class findScaleSpaceExtremaComputer : public ParallelLoopBody
{
public:
    findScaleSpaceExtremaComputer(
        int _o,
        int _i,
        int _threshold,
        int _idx,
        int _step,
        int _rows,
        int _cols,
        int _nOctaveLayers,
        double _contrastThreshold,
        double _edgeThreshold,
        double _sigma,
        const std::vector<Mat>& _gauss_pyr,
        const std::vector<Mat>& _dog_pyr,
        std::vector<KeyPoint>& _keypoints,
        Mutex &_mutex)

        : o(_o),
          i(_i),
          threshold(_threshold),
          idx(_idx),
          step(_step),
          rows(_rows),
          cols(_cols),
          nOctaveLayers(_nOctaveLayers),
          contrastThreshold(_contrastThreshold),
          edgeThreshold(_edgeThreshold),
          sigma(_sigma),
          gauss_pyr(_gauss_pyr),
          dog_pyr(_dog_pyr),
          keypoints(_keypoints),
          mutex(_mutex) { }

    void operator()( const cv::Range& range ) const
    {
        const int begin = range.start;
        const int end = range.end;

        static const int n = SIFT_ORI_HIST_BINS;
        float hist[n];

        const Mat& img = dog_pyr[idx];
        const Mat& prev = dog_pyr[idx-1];
        const Mat& next = dog_pyr[idx+1];

        //KeyPointsTLSType::reference myTLS = KeyPointsTLS.local();
        //myTLS.clear();

        for( int r = begin; r < end; r++)
        {
            KeyPoint kpt;

            const sift_wt* currptr = img.ptr<sift_wt>(r);
            const sift_wt* prevptr = prev.ptr<sift_wt>(r);
            const sift_wt* nextptr = next.ptr<sift_wt>(r);

            for( int c = SIFT_IMG_BORDER; c < cols-SIFT_IMG_BORDER; c++)
            {
                sift_wt val = currptr[c];

                // find local extrema with pixel accuracy
                if( std::abs(val) > threshold &&
                   ((val > 0 && val >= currptr[c-1] && val >= currptr[c+1] &&
                     val >= currptr[c-step-1] && val >= currptr[c-step] && val >= currptr[c-step+1] &&
                     val >= currptr[c+step-1] && val >= currptr[c+step] && val >= currptr[c+step+1] &&
                     val >= nextptr[c] && val >= nextptr[c-1] && val >= nextptr[c+1] &&
                     val >= nextptr[c-step-1] && val >= nextptr[c-step] && val >= nextptr[c-step+1] &&
                     val >= nextptr[c+step-1] && val >= nextptr[c+step] && val >= nextptr[c+step+1] &&
                     val >= prevptr[c] && val >= prevptr[c-1] && val >= prevptr[c+1] &&
                     val >= prevptr[c-step-1] && val >= prevptr[c-step] && val >= prevptr[c-step+1] &&
                     val >= prevptr[c+step-1] && val >= prevptr[c+step] && val >= prevptr[c+step+1]) ||
                     (val < 0 && val <= currptr[c-1] && val <= currptr[c+1] &&
                     val <= currptr[c-step-1] && val <= currptr[c-step] && val <= currptr[c-step+1] &&
                     val <= currptr[c+step-1] && val <= currptr[c+step] && val <= currptr[c+step+1] &&
                     val <= nextptr[c] && val <= nextptr[c-1] && val <= nextptr[c+1] &&
                     val <= nextptr[c-step-1] && val <= nextptr[c-step] && val <= nextptr[c-step+1] &&
                     val <= nextptr[c+step-1] && val <= nextptr[c+step] && val <= nextptr[c+step+1] &&
                     val <= prevptr[c] && val <= prevptr[c-1] && val <= prevptr[c+1] &&
                     val <= prevptr[c-step-1] && val <= prevptr[c-step] && val <= prevptr[c-step+1] &&
                     val <= prevptr[c+step-1] && val <= prevptr[c+step] && val <= prevptr[c+step+1])))
                {
                    int r1 = r, c1 = c, layer = i;
                    if( !adjustLocalExtrema(dog_pyr, kpt, o, layer, r1, c1,
                                            nOctaveLayers, (float)contrastThreshold,
                                            (float)edgeThreshold, (float)sigma) )
                        continue;
                    float scl_octv = kpt.size*0.5f/(1 << o);
                    float omax = calcOrientationHist(gauss_pyr[o*(nOctaveLayers+3) + layer],
                                                     Point(c1, r1),
                                                     cvRound(SIFT_ORI_RADIUS * scl_octv),
                                                     SIFT_ORI_SIG_FCTR * scl_octv,
                                                     hist, n);
                    float mag_thr = (float)(omax * SIFT_ORI_PEAK_RATIO);
                    for( int j = 0; j < n; j++ )
                    {
                        int l = j > 0 ? j - 1 : n - 1;
                        int r2 = j < n-1 ? j + 1 : 0;

                        if( hist[j] > hist[l]  &&  hist[j] > hist[r2]  &&  hist[j] >= mag_thr )
                        {
                            float bin = j + 0.5f * (hist[l]-hist[r2]) / (hist[l] - 2*hist[j] + hist[r2]);
                            bin = bin < 0 ? n + bin : bin >= n ? bin - n : bin;
                            kpt.angle = 360.f - (float)((360.f/n) * bin);
                           if(std::abs(kpt.angle - 360.f) < FLT_EPSILON)
                                kpt.angle = 0.f;

                            {
                                mutex.lock();
                                keypoints.push_back(kpt);
                                mutex.unlock();
                                //myTLS.push_back(kpt);
                            }
                        }
                    }
                }
            }
        }
    }

private:
    int o, i;
    int threshold;
    int idx, step, rows, cols;
    int nOctaveLayers;
    double contrastThreshold;
    double edgeThreshold;
    double sigma;
    const std::vector<Mat>& gauss_pyr;
    const std::vector<Mat>& dog_pyr;
    std::vector<KeyPoint>& keypoints;
    Mutex &mutex;
};

//
// Detects features at extrema in DoG scale space.  Bad features are discarded
// based on contrast and ratio of principal curvatures.
void SIFT_Impl::findScaleSpaceExtrema( const std::vector<Mat>& gauss_pyr, const std::vector<Mat>& dog_pyr,
                                  std::vector<KeyPoint>& keypoints ) const
{
    int nOctaves = (int)gauss_pyr.size()/(nOctaveLayers + 3);
    const int threshold = cvFloor(0.5 * contrastThreshold / nOctaveLayers * 255 * SIFT_FIXPT_SCALE);
    KeyPoint kpt;

    keypoints.clear();

    /*for (KeyPointsTLSType::iterator i = KeyPointsTLS.begin();
          i != KeyPointsTLS.end(); ++i)
    {
        i->clear();
    }*/

    Mutex mutex;

    for( int o = 0; o < nOctaves; o++ )
        for( int i = 1; i <= nOctaveLayers; i++ )
        {
            int idx = o*(nOctaveLayers+2)+i;
            const Mat& img = dog_pyr[idx];
            int step = (int)img.step1();
            int rows = img.rows, cols = img.cols;

            parallel_for_(Range(SIFT_IMG_BORDER, rows-SIFT_IMG_BORDER),
                    findScaleSpaceExtremaComputer(o, i, threshold, idx, step, rows, cols,
                        nOctaveLayers,
                        contrastThreshold,
                        edgeThreshold,
                        sigma,
                        gauss_pyr, dog_pyr, keypoints, mutex));
        }

    /*for (KeyPointsTLSType::const_iterator i = KeyPointsTLS.begin();
          i != KeyPointsTLS.end(); ++i)
     {
        keypoints.insert(keypoints.end(), i->begin(), i->end());
    }*/
}


static void calcSIFTDescriptor( const Mat& img, Point2f ptf, float ori, float scl,
                               int d, int n, float* dst )
{
    Point pt(cvRound(ptf.x), cvRound(ptf.y));
    float cos_t = cosf(ori*(float)(CV_PI/180));
    float sin_t = sinf(ori*(float)(CV_PI/180));
    float bins_per_rad = n / 360.f;
    float exp_scale = -1.f/(d * d * 0.5f);
    float hist_width = SIFT_DESCR_SCL_FCTR * scl;
    int radius = cvRound(hist_width * 1.4142135623730951f * (d + 1) * 0.5f);
    // Clip the radius to the diagonal of the image to avoid autobuffer too large exception
    radius = std::min(radius, (int) sqrt((double) img.cols*img.cols + img.rows*img.rows));
    cos_t /= hist_width;
    sin_t /= hist_width;

    int i, j, k, len = (radius*2+1)*(radius*2+1), histlen = (d+2)*(d+2)*(n+2);
    int rows = img.rows, cols = img.cols;

    AutoBuffer<float> buf(len*6 + histlen);
    float *X = buf, *Y = X + len, *Mag = Y, *Ori = Mag + len, *W = Ori + len;
    float *RBin = W + len, *CBin = RBin + len, *hist = CBin + len;

    std::memset(hist, 0, histlen * sizeof(float));

    for( i = -radius, k = 0; i <= radius; i++ )
        for( j = -radius; j <= radius; j++ )
        {
            // Calculate sample's histogram array coords rotated relative to ori.
            // Subtract 0.5 so samples that fall e.g. in the center of row 1 (i.e.
            // r_rot = 1.5) have full weight placed in row 1 after interpolation.
            float c_rot = j * cos_t - i * sin_t;
            float r_rot = j * sin_t + i * cos_t;
            float rbin = r_rot + d/2 - 0.5f;
            float cbin = c_rot + d/2 - 0.5f;
            int r = pt.y + i, c = pt.x + j;

            if( rbin > -1 && rbin < d && cbin > -1 && cbin < d &&
                r > 0 && r < rows - 1 && c > 0 && c < cols - 1 )
            {
                float dx = (float)(img.at<sift_wt>(r, c+1) - img.at<sift_wt>(r, c-1));
                float dy = (float)(img.at<sift_wt>(r-1, c) - img.at<sift_wt>(r+1, c));
                X[k] = dx; Y[k] = dy; RBin[k] = rbin; CBin[k] = cbin;
                W[k] = (c_rot * c_rot + r_rot * r_rot)*exp_scale;
                k++;
            }
        }

    len = k;
    cv::hal::fastAtan2(Y, X, Ori, len, true);
    cv::hal::magnitude32f(X, Y, Mag, len);
    cv::hal::exp32f(W, W, len);

    k = 0;
#if CV_AVX2
    int CV_DECL_ALIGNED(32) idx_buf[8];
    float CV_DECL_ALIGNED(32) val_buf[8];
    __m256 __ori = _mm256_set1_ps(ori);
    __m256 __bins_per_rad = _mm256_set1_ps(bins_per_rad);
    __m256i __n = _mm256_set1_epi32(n);
    for( ; k <= len - 8; k+=8 )
    {
        __m256 __rbin = _mm256_loadu_ps(&RBin[k]);
        __m256 __cbin = _mm256_loadu_ps(&CBin[k]);
        __m256 __obin = _mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(&Ori[k]), __ori), __bins_per_rad);
        __m256 __mag = _mm256_mul_ps(_mm256_loadu_ps(&Mag[k]), _mm256_loadu_ps(&W[k]));

        __m256 __r0 = _mm256_floor_ps(__rbin);
        __rbin = _mm256_sub_ps(__rbin, __r0);
        __m256 __c0 = _mm256_floor_ps(__cbin);
        __cbin = _mm256_sub_ps(__cbin, __c0);
        __m256 __o0 = _mm256_floor_ps(__obin);
        __obin = _mm256_sub_ps(__obin, __o0);

        __m256i __o0i = _mm256_cvtps_epi32(__o0);
        // _o0 += (o0 < 0) * n
        __o0i = _mm256_add_epi32(__o0i, _mm256_and_si256(
            __n,
            _mm256_cmpgt_epi32(_mm256_setzero_si256(), __o0i)));
        __o0i = _mm256_sub_epi32(__o0i, _mm256_and_si256(
            __n, 
            _mm256_or_si256(
                _mm256_cmpeq_epi32(__o0i, __n),
                _mm256_cmpgt_epi32(__o0i, __n))));

        __m256 __v_r1 = _mm256_mul_ps(__mag, __rbin);
        __m256 __v_r0 = _mm256_sub_ps(__mag, __v_r1);

        __m256 __v_rc11 = _mm256_mul_ps(__v_r1, __cbin);
        __m256 __v_rc10 = _mm256_sub_ps(__v_r1, __v_rc11);

        __m256 __v_rc01 = _mm256_mul_ps(__v_r0, __cbin);
        __m256 __v_rc00 = _mm256_sub_ps(__v_r0, __v_rc01);

        __m256 __v_rco111 = _mm256_mul_ps(__v_rc11, __obin);
        __m256 __v_rco110 = _mm256_sub_ps(__v_rc11, __v_rco111);

        __m256 __v_rco101 = _mm256_mul_ps(__v_rc10, __obin);
        __m256 __v_rco100 = _mm256_sub_ps(__v_rc10, __v_rco101);

        __m256 __v_rco011 = _mm256_mul_ps(__v_rc01, __obin);
        __m256 __v_rco010 = _mm256_sub_ps(__v_rc01, __v_rco011);

        __m256 __v_rco001 = _mm256_mul_ps(__v_rc00, __obin);
        __m256 __v_rco000 = _mm256_sub_ps(__v_rc00, __v_rco001);

        __m256i __one = _mm256_set1_epi32(1);
        __m256i __idx = 
            _mm256_add_epi32(
                _mm256_mullo_epi32(
                    _mm256_add_epi32(
                        _mm256_mullo_epi32(_mm256_add_epi32(_mm256_cvtps_epi32(__r0), __one), _mm256_set1_epi32(d + 2)),
                        _mm256_add_epi32(_mm256_cvtps_epi32(__c0), __one)),
                    _mm256_set1_epi32(n + 2)),
                __o0i);

        _mm256_store_si256((__m256i *)idx_buf, __idx);

#if 0
        #define SUM_HELPER(offset, __v)                \
            _mm256_store_ps(val_buf, __v);             \
            hist[idx_buf[0] + (offset)] += val_buf[0]; \
            hist[idx_buf[1] + (offset)] += val_buf[1]; \
            hist[idx_buf[2] + (offset)] += val_buf[2]; \
            hist[idx_buf[3] + (offset)] += val_buf[3]; \
            hist[idx_buf[4] + (offset)] += val_buf[4]; \
            hist[idx_buf[5] + (offset)] += val_buf[5]; \
            hist[idx_buf[6] + (offset)] += val_buf[6]; \
            hist[idx_buf[7] + (offset)] += val_buf[7]; \
        
        SUM_HELPER(0, __v_rco000);
        SUM_HELPER(1, __v_rco001);
        SUM_HELPER(n+2, __v_rco010);
        SUM_HELPER(n+3, __v_rco011);
        SUM_HELPER((d+2)*(n+2), __v_rco100);
        SUM_HELPER((d+2)*(n+2)+1, __v_rco101);
        SUM_HELPER((d+3)*(n+2), __v_rco110);
        SUM_HELPER((d+3)*(n+2)+1, __v_rco111);

        #undef SUM_HELPER
#else
        hist[idx_buf[0]] +=               __v_rco000[0];
        hist[idx_buf[0]+1] +=             __v_rco001[0];
        hist[idx_buf[0]+(n+2)] +=         __v_rco010[0];
        hist[idx_buf[0]+(n+3)] +=         __v_rco011[0];
        hist[idx_buf[0]+(d+2)*(n+2)]   += __v_rco100[0];
        hist[idx_buf[0]+(d+2)*(n+2)+1] += __v_rco101[0];
        hist[idx_buf[0]+(d+3)*(n+2)] +=   __v_rco110[0];
        hist[idx_buf[0]+(d+3)*(n+2)+1] += __v_rco111[0];

        hist[idx_buf[1]] +=               __v_rco000[1];
        hist[idx_buf[1]+1] +=             __v_rco001[1];
        hist[idx_buf[1]+(n+2)] +=         __v_rco010[1];
        hist[idx_buf[1]+(n+3)] +=         __v_rco011[1];
        hist[idx_buf[1]+(d+2)*(n+2)]   += __v_rco100[1];
        hist[idx_buf[1]+(d+2)*(n+2)+1] += __v_rco101[1];
        hist[idx_buf[1]+(d+3)*(n+2)] +=   __v_rco110[1];
        hist[idx_buf[1]+(d+3)*(n+2)+1] += __v_rco111[1];

        hist[idx_buf[2]] +=               __v_rco000[2];
        hist[idx_buf[2]+1] +=             __v_rco001[2];
        hist[idx_buf[2]+(n+2)] +=         __v_rco010[2];
        hist[idx_buf[2]+(n+3)] +=         __v_rco011[2];
        hist[idx_buf[2]+(d+2)*(n+2)]   += __v_rco100[2];
        hist[idx_buf[2]+(d+2)*(n+2)+1] += __v_rco101[2];
        hist[idx_buf[2]+(d+3)*(n+2)] +=   __v_rco110[2];
        hist[idx_buf[2]+(d+3)*(n+2)+1] += __v_rco111[2];

        hist[idx_buf[3]] +=               __v_rco000[3];
        hist[idx_buf[3]+1] +=             __v_rco001[3];
        hist[idx_buf[3]+(n+2)] +=         __v_rco010[3];
        hist[idx_buf[3]+(n+3)] +=         __v_rco011[3];
        hist[idx_buf[3]+(d+2)*(n+2)]   += __v_rco100[3];
        hist[idx_buf[3]+(d+2)*(n+2)+1] += __v_rco101[3];
        hist[idx_buf[3]+(d+3)*(n+2)] +=   __v_rco110[3];
        hist[idx_buf[3]+(d+3)*(n+2)+1] += __v_rco111[3];

        hist[idx_buf[4]] +=               __v_rco000[4];
        hist[idx_buf[4]+1] +=             __v_rco001[4];
        hist[idx_buf[4]+(n+2)] +=         __v_rco010[4];
        hist[idx_buf[4]+(n+3)] +=         __v_rco011[4];
        hist[idx_buf[4]+(d+2)*(n+2)]   += __v_rco100[4];
        hist[idx_buf[4]+(d+2)*(n+2)+1] += __v_rco101[4];
        hist[idx_buf[4]+(d+3)*(n+2)] +=   __v_rco110[4];
        hist[idx_buf[4]+(d+3)*(n+2)+1] += __v_rco111[4];

        hist[idx_buf[5]] +=               __v_rco000[5];
        hist[idx_buf[5]+1] +=             __v_rco001[5];
        hist[idx_buf[5]+(n+2)] +=         __v_rco010[5];
        hist[idx_buf[5]+(n+3)] +=         __v_rco011[5];
        hist[idx_buf[5]+(d+2)*(n+2)]   += __v_rco100[5];
        hist[idx_buf[5]+(d+2)*(n+2)+1] += __v_rco101[5];
        hist[idx_buf[5]+(d+3)*(n+2)] +=   __v_rco110[5];
        hist[idx_buf[5]+(d+3)*(n+2)+1] += __v_rco111[5];

        hist[idx_buf[6]] +=               __v_rco000[6];
        hist[idx_buf[6]+1] +=             __v_rco001[6];
        hist[idx_buf[6]+(n+2)] +=         __v_rco010[6];
        hist[idx_buf[6]+(n+3)] +=         __v_rco011[6];
        hist[idx_buf[6]+(d+2)*(n+2)]   += __v_rco100[6];
        hist[idx_buf[6]+(d+2)*(n+2)+1] += __v_rco101[6];
        hist[idx_buf[6]+(d+3)*(n+2)] +=   __v_rco110[6];
        hist[idx_buf[6]+(d+3)*(n+2)+1] += __v_rco111[6];

        hist[idx_buf[7]] +=               __v_rco000[7];
        hist[idx_buf[7]+1] +=             __v_rco001[7];
        hist[idx_buf[7]+(n+2)] +=         __v_rco010[7];
        hist[idx_buf[7]+(n+3)] +=         __v_rco011[7];
        hist[idx_buf[7]+(d+2)*(n+2)]   += __v_rco100[7];
        hist[idx_buf[7]+(d+2)*(n+2)+1] += __v_rco101[7];
        hist[idx_buf[7]+(d+3)*(n+2)] +=   __v_rco110[7];
        hist[idx_buf[7]+(d+3)*(n+2)+1] += __v_rco111[7];
#endif

    }
#endif
    for( ; k < len; k++ )
    {
        float rbin = RBin[k], cbin = CBin[k];
        float obin = (Ori[k] - ori)*bins_per_rad;
        float mag = Mag[k]*W[k];

        int r0 = cvFloor( rbin );
        int c0 = cvFloor( cbin );
        int o0 = cvFloor( obin );
        rbin -= r0;
        cbin -= c0;
        obin -= o0;

        if( o0 < 0 )
            o0 += n;
        if( o0 >= n )
            o0 -= n;

        // histogram update using tri-linear interpolation
        float v_r1 = mag*rbin, v_r0 = mag - v_r1;
        float v_rc11 = v_r1*cbin, v_rc10 = v_r1 - v_rc11;
        float v_rc01 = v_r0*cbin, v_rc00 = v_r0 - v_rc01;
        float v_rco111 = v_rc11*obin, v_rco110 = v_rc11 - v_rco111;
        float v_rco101 = v_rc10*obin, v_rco100 = v_rc10 - v_rco101;
        float v_rco011 = v_rc01*obin, v_rco010 = v_rc01 - v_rco011;
        float v_rco001 = v_rc00*obin, v_rco000 = v_rc00 - v_rco001;

        int idx = ((r0+1)*(d+2) + c0+1)*(n+2) + o0;
        hist[idx] += v_rco000;
        hist[idx+1] += v_rco001;
        hist[idx+(n+2)] += v_rco010;
        hist[idx+(n+3)] += v_rco011;
        hist[idx+(d+2)*(n+2)] += v_rco100;
        hist[idx+(d+2)*(n+2)+1] += v_rco101;
        hist[idx+(d+3)*(n+2)] += v_rco110;
        hist[idx+(d+3)*(n+2)+1] += v_rco111;
    }

    // finalize histogram, since the orientation histograms are circular
    for( i = 0; i < d; i++ )
        for( j = 0; j < d; j++ )
        {
            int idx = ((i+1)*(d+2) + (j+1))*(n+2);
            hist[idx] += hist[idx+n];
            hist[idx+1] += hist[idx+n+1];

            std::memcpy(&dst[(i*d + j)*n], &hist[idx], n * sizeof(float));
            //for( k = 0; k < n; k++ )
            //    dst[(i*d + j)*n + k] = hist[idx+k];
        }
    // copy histogram to the descriptor,
    // apply hysteresis thresholding
    // and scale the result, so that it can be easily converted
    // to byte array
    float nrm2 = 0;
    len = d*d*n;
    CV_Assert(len == 128); // Divisible by 8
#if CV_AVX2
    __m256 __nrm2 = _mm256_setzero_ps();
    __m256 __dst;
    for( k = 0; k <= len - 8; k += 8 )
    {
        __dst = _mm256_loadu_ps(&dst[k]);
#if CV_FMA3
        __nrm2 = _mm256_fmadd_ps(__dst, __dst, __nrm2);
#else
        __nrm2 = _mm256_add_ps(__nrm2, _mm256_mul_ps(__dst, __dst));
#endif
    }
    _mm256_store_ps(val_buf, __nrm2);
    nrm2 = ((val_buf[0] + val_buf[1]) + (val_buf[2] + val_buf[3])) +
           ((val_buf[4] + val_buf[5]) + (val_buf[6] + val_buf[7]));
#else
    for( k = 0; k < len; k++ )
        nrm2 += dst[k]*dst[k];
#endif

    float thr = std::sqrt(nrm2)*SIFT_DESCR_MAG_THR;

#if CV_AVX2
    __nrm2 = _mm256_setzero_ps();
    __m256 __thr = _mm256_set1_ps(thr);
    for( k = 0; k <= len - 8; k += 8 )
    {
        __dst = _mm256_loadu_ps(&dst[k]);
        __dst = _mm256_min_ps(__dst, __thr);
        _mm256_storeu_ps(&dst[k], __dst);
#if CV_FMA3
        __nrm2 = _mm256_fmadd_ps(__dst, __dst, __nrm2);
#else
        __nrm2 = _mm256_add_ps(__nrm2, _mm256_mul_ps(__dst, __dst));
#endif
    }
    _mm256_store_ps(val_buf, __nrm2);
    nrm2 = ((val_buf[0] + val_buf[1]) + (val_buf[2] + val_buf[3])) +
           ((val_buf[4] + val_buf[5]) + (val_buf[6] + val_buf[7]));
#else
    for( i = 0, nrm2 = 0; i < k; i++ )
    {
        float val = std::min(dst[i], thr);
        dst[i] = val;
        nrm2 += val*val;
    }
#endif
    nrm2 = SIFT_INT_DESCR_FCTR/std::max(std::sqrt(nrm2), FLT_EPSILON);

#if 1
#if CV_AVX2
    __m256 __min = _mm256_setzero_ps();
    __m256 __max = _mm256_set1_ps(255.0f); // max of uchar
    __nrm2 = _mm256_set1_ps(nrm2);
    for( k = 0; k <= len - 8; k+=8 )
    {
        __dst = _mm256_loadu_ps(&dst[k]);
        __dst = _mm256_min_ps(_mm256_max_ps(_mm256_floor_ps(_mm256_mul_ps(__dst, __nrm2)), __min), __max);
        _mm256_storeu_ps(&dst[k], __dst);
    }
#else
    for( k = 0; k < len; k++ )
    {
        dst[k] = saturate_cast<uchar>(dst[k]*nrm2);
    }
#endif
#else
    float nrm1 = 0;
    for( k = 0; k < len; k++ )
    {
        dst[k] *= nrm2;
        nrm1 += dst[k];
    }
    nrm1 = 1.f/std::max(nrm1, FLT_EPSILON);
    for( k = 0; k < len; k++ )
    {
        dst[k] = std::sqrt(dst[k] * nrm1);//saturate_cast<uchar>(std::sqrt(dst[k] * nrm1)*SIFT_INT_DESCR_FCTR);
    }
#endif
}

class calcDescriptorsComputer : public ParallelLoopBody
{
public:
    calcDescriptorsComputer(const std::vector<Mat>& _gpyr,
                            const std::vector<KeyPoint>& _keypoints,
                            Mat& _descriptors,
                            int _nOctaveLayers,
                            int _firstOctave)
        : gpyr(_gpyr),
          keypoints(_keypoints),
          descriptors(_descriptors),
          nOctaveLayers(_nOctaveLayers),
          firstOctave(_firstOctave) { }

    void operator()( const cv::Range& range ) const
    {
        const int begin = range.start;
        const int end = range.end;

        static const int d = SIFT_DESCR_WIDTH, n = SIFT_DESCR_HIST_BINS;

        for ( int i = begin; i<end; i++ )
        {
            KeyPoint kpt = keypoints[i];
            int octave, layer;
            float scale;
            unpackOctave(kpt, octave, layer, scale);
            CV_Assert(octave >= firstOctave && layer <= nOctaveLayers+2);
            float size=kpt.size*scale;
            Point2f ptf(kpt.pt.x*scale, kpt.pt.y*scale);
            const Mat& img = gpyr[(octave - firstOctave)*(nOctaveLayers + 3) + layer];

            float angle = 360.f - kpt.angle;
            if(std::abs(angle - 360.f) < FLT_EPSILON)
                angle = 0.f;
            calcSIFTDescriptor(img, ptf, angle, size*0.5f, d, n, descriptors.ptr<float>((int)i));
        }
    }

private:
    const std::vector<Mat>& gpyr;
    const std::vector<KeyPoint>& keypoints;
    Mat& descriptors;
    int nOctaveLayers;
    int firstOctave;
};

static void calcDescriptors(const std::vector<Mat>& gpyr, const std::vector<KeyPoint>& keypoints,
                            Mat& descriptors, int nOctaveLayers, int firstOctave )
{
    parallel_for_(Range(0, keypoints.size()), calcDescriptorsComputer(gpyr, keypoints, descriptors, nOctaveLayers, firstOctave));
}

//////////////////////////////////////////////////////////////////////////////////////////

SIFT_Impl::SIFT_Impl( int _nfeatures, int _nOctaveLayers,
           double _contrastThreshold, double _edgeThreshold, double _sigma, bool _useNegativeOctave )
    : nfeatures(_nfeatures), nOctaveLayers(_nOctaveLayers),
      contrastThreshold(_contrastThreshold), edgeThreshold(_edgeThreshold),
      sigma(_sigma), useNegativeOctave(_useNegativeOctave)
{
}

int SIFT_Impl::descriptorSize() const
{
    return SIFT_DESCR_WIDTH*SIFT_DESCR_WIDTH*SIFT_DESCR_HIST_BINS;
}

int SIFT_Impl::descriptorType() const
{
    return CV_32F;
}

int SIFT_Impl::defaultNorm() const
{
    return NORM_L2;
}

void SIFT_Impl::detectAndCompute(InputArray _image, InputArray _mask,
                      std::vector<KeyPoint>& keypoints,
                      OutputArray _descriptors,
                      bool useProvidedKeypoints)
{
    int firstOctave = useNegativeOctave ? -1 : 0;
    int actualNOctaves = 0, actualNLayers = 0;
    Mat image = _image.getMat(), mask = _mask.getMat();

    if( image.empty() || image.depth() != CV_8U )
        CV_Error( Error::StsBadArg, "image is empty or has incorrect depth (!=CV_8U)" );

    if( !mask.empty() && mask.type() != CV_8UC1 )
        CV_Error( Error::StsBadArg, "mask has incorrect type (!=CV_8UC1)" );

    if( useProvidedKeypoints )
    {
        firstOctave = 0;
        int maxOctave = INT_MIN;
        for( size_t i = 0; i < keypoints.size(); i++ )
        {
            int octave, layer;
            float scale;
            unpackOctave(keypoints[i], octave, layer, scale);
            firstOctave = std::min(firstOctave, octave);
            maxOctave = std::max(maxOctave, octave);
            actualNLayers = std::max(actualNLayers, layer-2);
        }

        firstOctave = std::min(firstOctave, 0);
        CV_Assert( firstOctave >= -1 && actualNLayers <= nOctaveLayers );
        actualNOctaves = maxOctave - firstOctave + 1;
    }

    Mat base = createInitialImage(image, firstOctave < 0, (float)sigma);
    std::vector<Mat> gpyr, dogpyr;
    int nOctaves = actualNOctaves > 0 ? actualNOctaves : cvRound(std::log( (double)std::min( base.cols, base.rows ) ) / std::log(2.) - 2) - firstOctave;

    //double t, tf = getTickFrequency();
    //t = (double)getTickCount();
    buildGaussianPyramid(base, gpyr, nOctaves);
    //t = (double)getTickCount() - t;
    //printf("pyramid construction time: %g\n", t*1000./tf);
    //t = (double)getTickCount();
    buildDoGPyramid(gpyr, dogpyr);

    //t = (double)getTickCount() - t;
    //printf("pyramid construction time: %g\n", t*1000./tf);

    if( !useProvidedKeypoints )
    {
        //t = (double)getTickCount();
        findScaleSpaceExtrema(gpyr, dogpyr, keypoints);
        KeyPointsFilter::removeDuplicated( keypoints );

        if( !mask.empty() )
            KeyPointsFilter::runByPixelsMask( keypoints, mask );

        if( nfeatures > 0 )
            KeyPointsFilter::retainBest(keypoints, nfeatures);
        //t = (double)getTickCount() - t;
        //printf("keypoint detection time: %g\n", t*1000./tf);

        if( firstOctave < 0 )
            for( size_t i = 0; i < keypoints.size(); i++ )
            {
                KeyPoint& kpt = keypoints[i];
                float scale = 1.f/(float)(1 << -firstOctave);
                kpt.octave = (kpt.octave & ~255) | ((kpt.octave + firstOctave) & 255);
                kpt.pt *= scale;
                kpt.size *= scale;
            }
    }
    else
    {
        // filter keypoints by mask
        //KeyPointsFilter::runByPixelsMask( keypoints, mask );
    }
    //clock_t begin = clock();
    if( _descriptors.needed() )
    {
        //t = (double)getTickCount();
        int dsize = descriptorSize();
        _descriptors.create((int)keypoints.size(), dsize, CV_32F);
        Mat descriptors = _descriptors.getMat();

        calcDescriptors(gpyr, keypoints, descriptors, nOctaveLayers, firstOctave);
        //t = (double)getTickCount() - t;
        //printf("descriptor extraction time: %g\n", t*1000./tf);
    }
    //printf("calcDesc %f\n", double(clock() - begin) / CLOCKS_PER_SEC);
}

}
}
