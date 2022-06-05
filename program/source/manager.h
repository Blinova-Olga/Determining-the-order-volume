#pragma once

#include "models.h"
#include "equations.h"
#include <vector>
#include <optional>
#include <limits>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <chrono>
#include <ctime>

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

std::vector<std::pair<time_t, double>> readFile(std::string filename) {
    std::ifstream file(filename);
    std::vector<std::pair<time_t, double>> result;
    if (file.is_open()) {
        std::string line;
        if (!(getline(file, line) && line.find("Purchase Forecast file") != std::string::npos)) {
            file.close();
            return {};
        }
        tm date {};
        double y{};
        while (getline(file, line)) {
            std::istringstream input(line);
            input >> std::get_time(&date, "%d.%m.%Y") >> y;
            time_t d = mktime(&date);
            result.emplace_back(d, y);
        }
        file.close();
    }
    return result;
}

std::vector<GaussDestrParameters> parsePeriods(const std::vector<std::pair<time_t, double>> &raw, Period period) {
    std::vector<GaussDestrParameters> result;
    int hours = 24;
    switch (period) {
        case Period::day:
            break;
        case Period::week:
            hours *= 7;
            break;
        case Period::month:
            hours *= 30;
            break;
    }
    if(raw.size() == 0){
        return {};
    }
    auto p = std::chrono::hours(hours);
    double sum = 0.0;
    int count = 0;
    auto now = std::chrono::system_clock::from_time_t(raw[0].first);
    auto nextPeriod = now + p;
    std::vector<double> sigma;
    for (const auto &[date, y]: raw) {
        if(y == 0.0){
            continue;
        }
        if (date < std::chrono::system_clock::to_time_t(nextPeriod)) {
            sum += y;
            sigma.push_back(y);
        } else {
            nextPeriod += p;
            double stdErr;
            double variance{0.0};
            double sigma_gauss = sum / 3.0;
            if(sigma.size() > 1){
                double average = sum / (sigma.size());
                for (const auto &x: sigma)
                    variance += (x - average) * (x - average);
                variance /= sigma.size() - 1;
                sigma_gauss = std::sqrt(variance) * sigma.size();
            }
            if(sigma_gauss == 0){
                sigma_gauss = 1.0;
            }
            result.push_back({sum, sigma_gauss});
            sum = y;
            sigma.clear();
            sigma.push_back(y);
        }
    }
    return result;
}

class TaskCalculator {
public:
    TaskCalculator(TaskParameters params, GaussDestrParameters destr, int periodsNum, int dotsNum)
            : m_params(params), m_gauss(destr), totalPeriods(periodsNum), N(dotsNum), y_max(periodsNum-1, destr.mean),
              x(linspace(-(m_gauss.mean + 3 * m_gauss.sigma), (m_gauss.mean + 3 * m_gauss.sigma), dotsNum)),
              convolution(m_gauss.mean, m_gauss.sigma, x), integrator(params, destr), profit_max(periodsNum-1, 0.0) {
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
        for(int i = res.size()-2;i>=0;--i){
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
        ans->MaxProfit = maxF - profit_max[0];
        ans->thisPeriodProfit = maxF - profit_max[0];
        std::vector<double> res = profit_max;
        for(int i = res.size()-2;i>=0;--i){
            res[i] = profit_max[i] - profit_max[i+1];
        }
        for(const auto& it : res){
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
        if(gaussParamsVector.size() > currentPeriod){
            convolution.setDistributionParams(gaussParamsVector[currentPeriod]);
            integrator.setDistributionParams(gaussParamsVector[currentPeriod]);
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
