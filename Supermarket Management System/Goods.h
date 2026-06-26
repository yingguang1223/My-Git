#pragma once
#include <string>
#include <iostream>
#include <fstream>
using namespace std;


class Goods {
protected:
    string name;
    int number;
    double price;

public:
    
    Goods(string n = "", int num = 0, double p = 0.0) 
        : name(n), number(num), price(p) {}

    
    virtual ~Goods() {}

    
    Goods(const Goods& other) 
        : name(other.name), number(other.number), price(other.price) {}

    
    Goods& operator=(const Goods& other) {
        if (this != &other) {
            name = other.name;
            number = other.number;
            price = other.price;
        }
        return *this;
    }

    
    bool operator==(const string& target) const {
        return name == target;
    }

    
    friend ostream& operator<<(ostream& out, const Goods& g) {
        g.print(out);  
        return out;
    }

    
    virtual void print(ostream& out) const = 0;
    virtual void saveToFile(ofstream& out) const = 0;
    virtual void loadFromFile(ifstream& in) = 0;

   
    string getName() const { return name; }
    int getNumber() const { return number; }
    double getPrice() const { return price; }
    void setPrice(double p) { price = p; }   
    void setNumber(int n) { number = n; }
    void addNumber(int n) { number += n; }
};