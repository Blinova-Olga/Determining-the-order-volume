#pragma once

struct Answer{
    double y;
    double thisPeriodProfit;
    double MaxProfit;
};

enum class Period{
    day,
    week,
    month,
};

struct TaskParameters {
    double profitOfOnePurchase;     // r
    double storageCosts;            // h
    double inflation;               // alpha
    double deficitCoefficient;      // p
    double purchasePrice;           // c
};

struct GaussDestrParameters{
    double mean;
    double sigma;
};


class ParameterizedFunction {
public:
    void setY(double y_) {
        y = y_;
    }
    virtual double operator()(double D) = 0;
    virtual ~ParameterizedFunction() {}
protected:
    double y{0};
};

class Integrator {
public:
    virtual double calculate(double y) const = 0;
    virtual ~Integrator() {}
};

