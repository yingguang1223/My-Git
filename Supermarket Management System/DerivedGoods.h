#pragma once
#include "Goods.h"
#include <fstream>
using namespace std;

class DailyGoods : public Goods {
public:
    DailyGoods(string n = "", int num = 0, double p = 0.0) : Goods(n, num, p) {}
    void print(ostream& out) const override;      
    void saveToFile(ofstream& out) const override;
    void loadFromFile(ifstream& in) override;
};


class Food : public Goods {
private:
public:
    Food(string n = "", int num = 0, double p = 0.0, int st = 0) 
        : Goods(n, num, p), saveTime(st) {}
    void print(ostream& out) const override;
    void saveToFile(ofstream& out) const override;
    void loadFromFile(ifstream& in) override;
};


class ElectricalAppliance : public Goods {
private:
    string color;
public:
    ElectricalAppliance(string n = "", int num = 0, double p = 0.0, string c = "") 
        : Goods(n, num, p), color(c) {}
    void print(ostream& out) const override;
    void saveToFile(ofstream& out) const override;
    void loadFromFile(ifstream& in) override;
};


void loadAllData();
void saveAllData();