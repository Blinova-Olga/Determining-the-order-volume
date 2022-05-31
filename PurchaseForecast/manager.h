#pragma once

#include "models.h"
#include "equations.h"
#include <vector>
#include <optional>
#include <limits>

template<typename T>
std::vector<T> linspace(T a, T b, size_t N) {
    T h = (b - a) / static_cast<T>(N - 1);
    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h)
        *x = val;
    return xs;
}

class TaskCalculator {
public:
    TaskCalculator(TaskParameters params, GaussDestrParameters destr, int periodsNum, int dotsNum)
            : m_params(params), m_gauss(destr), totalPeriods(periodsNum), N(dotsNum), y_max(periodsNum-1, destr.mean),
              x(linspace(-(m_gauss.mean + 3 * m_gauss.sigma), (m_gauss.mean + 3 * m_gauss.sigma), dotsNum)),
              convolution(m_gauss.mean, m_gauss.sigma, x), integrator(params, destr), profit_max(periodsNum-1) {
        F.resize(totalPeriods);
        for (auto &v: F) {
            v.resize(N);
        }
        F[totalPeriods - 1].assign(N, 0.0);
        currentPeriod = totalPeriods - 2;
        x_zero_pos = x.size() / 2;
    }

    int getCurrentPeriod() {
        return currentPeriod;
    }

    const std::vector<double> &getMaxY() {
        return y_max;
    }

    std::vector<double> getMaxProfit() {
        std::vector<double> res = profit_max;
        for(int i = res.size();i>=0;--i){
            res[i] = profit_max[i] - profit_max[i+1];
        }
        return res;
    }

    std::optional<Answer> getAnswer(double current_x) {
        if (currentPeriod >= 0 || current_x > x[x.size()-1]) {
            return std::nullopt;
        }
        if (ans) {
            return ans;
        }
        if (gaussParamsVector.size() > 0) {
            convolution.setDistributionParams(gaussParamsVector[0]);
            integrator.setDistributionParams(gaussParamsVector[0]);
        }
        double cur_y_max = -std::numeric_limits<double>::max();
        auto fft_F = convolution.calculate(F[0]);
        double maxF = -std::numeric_limits<double>::max();
        for (int i = x.size() - 1; i >= x_zero_pos; --i) {
            if(current_x > x[i]){
                break;
            }
            double sum_i = -m_params.purchasePrice * (x[i] - current_x)  + integrator.calculate(x[i]) +
                           m_params.inflation * fft_F[getIndexFromY(x[i])];
            if (sum_i > maxF) {
                maxF = sum_i;
                cur_y_max = x[i];
            }
        }
        ans->y = cur_y_max;
        ans->thisPeriodProfit = maxF;
        ans->MaxProfit = maxF;
        for(const auto& it : profit_max){
            ans->MaxProfit +=it;
        }
        return ans;
    }

    bool calcPeriod() {
        if (currentPeriod < 0) {
            return false;
        }
        auto &F_current = F[currentPeriod];
        int n = F_current.size();
        if(gaussParamsVector.size() > currentPeriod+1){
            convolution.setDistributionParams(gaussParamsVector[currentPeriod+1]);
            integrator.setDistributionParams(gaussParamsVector[currentPeriod+1]);
        }
        auto fft_F = convolution.calculate(F[currentPeriod + 1]);
        double maxF = -std::numeric_limits<double>::max();
        for (int i = n - 1; i >= x_zero_pos; --i) {
            double sum_i = -m_params.purchasePrice * x[i] + integrator.calculate(x[i]) +
                           m_params.inflation * fft_F[getIndexFromY(x[i])];


            if(sum_i > maxF){
                maxF = sum_i;
                y_max[currentPeriod] = x[i];
            }
            F_current[i] = maxF;
            if(y_max[currentPeriod] >= x[i]){
                F_current[i] += m_params.purchasePrice * x[i];
            }
        }
        profit_max[currentPeriod] = maxF;
        for (int i = x_zero_pos - 1; i >= 0; --i) {
            F_current[i] = maxF + m_params.purchasePrice * x[i];
        }
        currentPeriod--;

        return true;
    }

    void setGaussVector(std::vector<GaussDestrParameters> gaussParam){
        gaussParamsVector = std::move(gaussParam);
    }

private:

    int getIndexFromY(double y){
        return static_cast<int>( y / x[x.size() - 1] * static_cast<double>(x.size() / 2));
    }


private:
    TaskParameters m_params;
    GaussDestrParameters m_gauss;
    int currentPeriod;
    const int totalPeriods;
    const int N;
    std::vector<std::vector<double>> F;
    std::vector<double> y_max;
    std::vector<GaussDestrParameters> gaussParamsVector{};
    std::vector<double> profit_max;
    std::vector<double> x;
    size_t x_zero_pos;
    std::optional<Answer> ans{std::nullopt};
private:
    GaussConvolution convolution;
    ExplicitIntegrator integrator;
};

class manager {
public:
    void process();
};
