#pragma once

#include "models.h"

#include <fftw3.h>
#include <complex>
#include <vector>
#include <cmath>
#include <numbers>
#include <functional>
#include <algorithm>
#include <math.h>


class ExplicitIntegrator : public Integrator {

public:
    ExplicitIntegrator(TaskParameters params, GaussDestrParameters gauss)
            : r{params.profitOfOnePurchase}, h{params.storageCosts}, alpha{params.inflation},
              p{params.deficitCoefficient}, M{gauss.mean}, s{gauss.sigma} {
        updateConstants();
    }

    double calculate(double y) const override {
        double erfCurrent = std::erf((M - y) / (std::sqrt(2) * s)) / 2.0;
        double expCurrent = std::exp(-(M - y) * (M - y) / (2.0 * s * s)) * (s / Sqrt2Pi);

        double probFromZeroToY = (erfPermanent - erfCurrent) / densityCoefficient;
        double momentFromZeroToY = (M * (erfPermanent - erfCurrent) + expPermanent - expCurrent) / densityCoefficient;

        return (alpha * r - p) * firstMoment + ((1 - alpha) * r + p) * y
               + (-alpha * r + p + r + h) * momentFromZeroToY + ((alpha - 1) * r - p - h) * y * probFromZeroToY;
    }

    void setDistributionParams(GaussDestrParameters params) {
        M = params.mean;
        s = params.sigma;
        updateConstants();
    }

private:
    void updateConstants() {
        erfPermanent = std::erf(M / (std::sqrt(2.0) * s)) / 2.0;
        expPermanent = std::exp(-M * M / (2.0 * s * s)) * (s / Sqrt2Pi);
        densityCoefficient = 0.5 + erfPermanent;
        firstMoment = (expPermanent + M / 2.0 + M * erfPermanent) / densityCoefficient;
    }

private:
    double r;
    double h;
    double alpha;
    double p;
private:
    double M;
    double s;
    double erfPermanent;
    double expPermanent;
    double densityCoefficient;
    double firstMoment;
    const double Sqrt2Pi = std::sqrt(2.0 * M_PI);
};


class GaussConvolution {
public:
    GaussConvolution(double expectedValue, double sigma, const std::vector<double> &x_)
            : M{expectedValue}, s{sigma}, x(x_) {
        exp_series = makeExpSeries();
        x_coef = x[x.size() - 1] - x[0];
    }

    [[nodiscard]] std::vector<double> calculate(const std::vector<double> &source) const {
        assert(exp_series.size() == source.size());
        int n = exp_series.size();
        int m = n/2;

        std::vector<std::complex<double>> fft_exp(n);
        std::vector<std::complex<double>> fft_func(n);
        for (size_t i = 0; i < n; ++i) {
            fft_func[i] = source[i];
        }
        fftw_plan plan_exp = fftw_plan_dft_1d(fft_func.size(), (fftw_complex *) &fft_func[0], (fftw_complex *) &fft_func[0],
                                              FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_plan plan_func = fftw_plan_dft_1d(fft_exp.size(), (fftw_complex *) &exp_series[0], (fftw_complex *) &fft_exp[0],
                                               FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(plan_exp);
        fftw_execute(plan_func);
        std::vector<std::complex<double>> conv(n);
        for (size_t i = 0; i < conv.size(); ++i) {
            conv[i] = fft_exp[i] * fft_func[i];
        }
        fftw_plan plan_res = fftw_plan_dft_1d(conv.size(), (fftw_complex *) &conv[0], (fftw_complex *) &conv[0], FFTW_BACKWARD,
                                              FFTW_ESTIMATE);
        fftw_execute(plan_res);
        std::vector<double> result(m);
        const double coef = x_coef / (static_cast<double>(n) * static_cast<double>(n-1) * s *s);
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = real(conv[i]) * coef;
        }

        fftw_destroy_plan(plan_exp);
        fftw_destroy_plan(plan_func);
        fftw_destroy_plan(plan_res);
        return result;
    }

    void setDistributionParams(GaussDestrParameters params) {
        M = params.mean;
        s = params.sigma;
        NormCoeff = s / std::sqrt(2.0 * M_PI);
        ConstantInExp = -1.0 / (2.0 * s * s);
        exp_series = makeExpSeries();
    }

private:
    [[nodiscard]] std::vector<std::complex<double>> makeExpSeries() const {
        size_t m = x.size()/2;
        std::vector<std::complex<double>> result(x.size(), 0.0);
        for (size_t i = m; i < x.size(); ++i) {
            double arg = M - x[i];
            result[i] = std::exp(ConstantInExp * arg * arg) * NormCoeff;
        }
        return result;
    }

private:
    double M;
    double s;
    double x_coef;
    std::vector<std::complex<double>> exp_series;
    const std::vector<double> x;
    double NormCoeff = s / std::sqrt(2.0 * M_PI);
    double ConstantInExp = -1.0 / (2.0 * s * s);
};
