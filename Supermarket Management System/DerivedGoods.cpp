#include "DerivedGoods.h"
#include <stdexcept>   
using namespace std;


extern Goods* inventory[1000];
extern int inventoryCount;




void DailyGoods::print(ostream& out) const {
    out << name << " | 数量:" << number << " | 单价:" << price << "元";
}
void DailyGoods::saveToFile(ofstream& out) const {
    out << "1 " << name << " " << number << " " << price << "\n";   
}
void DailyGoods::loadFromFile(ifstream& in) {
    in >> name >> number >> price;
}


void Food::print(ostream& out) const {
    out << name << " | 数量:" << number << " | 单价:" << price 
        << "元 | 保质期:" << saveTime << "天";
}
void Food::saveToFile(ofstream& out) const {
    out << "2 " << name << " " << number << " " << price << " " << saveTime << "\n";
}
void Food::loadFromFile(ifstream& in) {
    in >> name >> number >> price >> saveTime;
}


void ElectricalAppliance::print(ostream& out) const {
    out << name << " | 数量:" << number << " | 单价:" << price 
        << "元 | 颜色:" << color;
}
void ElectricalAppliance::saveToFile(ofstream& out) const {
    out << "3 " << name << " " << number << " " << price << " " << color << "\n";
}
void ElectricalAppliance::loadFromFile(ifstream& in) {
    in >> name >> number >> price >> color;
}



void loadAllData() {
    ifstream in("./data/goods.dat");
    if (!in.is_open()) {
        
        return;
    }

    int type;
    while (in >> type) {  
        if (inventoryCount >= 1000) break;  

        Goods* g = nullptr;
        if (type == 1) {
            g = new DailyGoods();
        } else if (type == 2) {
            g = new Food();
        } else if (type == 3) {
            g = new ElectricalAppliance();
        } else {
            
            throw runtime_error("文件数据损坏：未知类型编号");
        }

        if (g) {
            g->loadFromFile(in);  
            inventory[inventoryCount++] = g;
        }
    }
    in.close();  
}

void saveAllData() {
    ofstream out("./data/goods.dat");
    if (!out) {
       
        throw runtime_error("无法打开文件保存");
    }
    for (int i = 0; i < inventoryCount; i++) {
        inventory[i]->saveToFile(out);  
    }
    out.close();
}