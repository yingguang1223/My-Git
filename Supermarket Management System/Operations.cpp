#include "Operations.h"
#include "DerivedGoods.h"   
#include <iostream>
#include <limits>  
using namespace std; 


extern Goods* inventory[1000];
extern int inventoryCount;

template<typename T>
T safeInput(const string& prompt) {
    T val;
    while (true) {
        cout << prompt;
        if (cin >> val) {   
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  
            return val;
        }
       
        cin.clear();  
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "输入无效，请重新输入！" << endl;
    }
}

void addItem() {
    int type = safeInput<int>("类型(1日用品/2食品/3家电): ");
    string name = safeInput<string>("名称: ");
    int num = safeInput<int>("数量: ");
    double price = safeInput<double>("单价: ");

  
    for (int i = 0; i < inventoryCount; i++) {
        if (*inventory[i] == name) {   
            inventory[i]->addNumber(num);
            cout << "已存在，数量合并为 " << inventory[i]->getNumber() << endl;
            return;
        }
    }

    
    Goods* newItem = nullptr;
    if (type == 1) {
        newItem = new DailyGoods(name, num, price);
    } else if (type == 2) {
        int st = safeInput<int>("保质期(天): ");
        newItem = new Food(name, num, price, st);
    } else if (type == 3) {
        string color = safeInput<string>("颜色: ");
        newItem = new ElectricalAppliance(name, num, price, color);
    } else {
        cout << "无效类型！" << endl;
        return;
    }

    if (inventoryCount < 1000) {
        inventory[inventoryCount++] =newItem; 
        cout << "添加成功！当前共 " << inventoryCount << " 种商品" << endl;
    } else {
        cout << "库存已满（上限1000种）！" << endl;
        delete newItem;  
    }
}

void sellItem() {
    string name = safeInput<string>("出售名称: ");
    int num = safeInput<int>("数量: ");

    int index = -1;
    for (int i = 0; i < inventoryCount; i++) {
        if (inventory[i]->getName() == name) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        cout << "未找到商品" << endl;
        return;
    }

    Goods* target = inventory[index];
    if (target->getNumber() < num) {
        cout << "库存不足！(现有 " << target->getNumber() << ")" << endl;
        return;
    }

    target->setNumber(target->getNumber() - num);

    if (target->getNumber() == 0) {
        delete target;  
        
        for (int i = index; i < inventoryCount - 1; i++) {
            inventory[i] = inventory[i + 1];
        }
        inventoryCount--;
        cout << "已售罄，商品已从系统中移除" << endl;
    } else {
        cout << "出售成功，剩余 " << target->getNumber() << endl;
    }
}


void searchItem() {
    string keyword = safeInput<string>("输入关键词: ");
    bool found = false;

    for (int i = 0; i < inventoryCount; i++) {
        // string::find 子串查找
        if (inventory[i]->getName().find(keyword) != string::npos) {
            cout << *inventory[i] << endl;   
            found = true;
        }
    }
    if (!found) cout << "无匹配结果" << endl;
}


void showAll() {
    if (inventoryCount == 0) {
        cout << "库存为空" << endl;
        return;
    }

    double total = 0.0;
    cout << "\n======= 库存清单 =======" << endl;
    for (int i = 0; i < inventoryCount; i++) {
        cout << *inventory[i] << endl;   
        
  
        if (inventory[i]->getNumber() < 5) {
            cout << "[警告]该商品库存不足(低于5件),请及时补货！" << endl;
        }
        
        total += inventory[i]->getNumber() * inventory[i]->getPrice();
    }
    cout << "库存总价值: " << total << " 元" << endl;
}


void updatePrice() {
    string name = safeInput<string>("请输入要修改价格的商品名称: ");
    
    for (int i = 0; i < inventoryCount; i++) {
        if (inventory[i]->getName() == name) {
            double newPrice = safeInput<double>("请输入新单价: ");
            inventory[i]->setPrice(newPrice);  // 调用 A 刚刚加的 setPrice
            cout << "单价修改成功！" << endl;
            return;
        }
    }
    cout << "未找到该商品！" << endl;
}


void searchByPriceRange() {
    double low = safeInput<double>("请输入最低价: ");
    double high = safeInput<double>("请输入最高价: ");

    if (low > high) {
        cout << "最低价不能高于最高价！" << endl;
        return;
    }

    bool found = false;
    cout << "\n===== 价格区间 [" << low << ", " << high << "] 商品列表 =====" << endl;
    for (int i = 0; i < inventoryCount; i++) {
        double p = inventory[i]->getPrice();
        if (p >= low && p <= high) {
            cout << *inventory[i] << endl;
            found = true;
        }
    }
    if (!found) cout << "该区间内没有商品。" << endl;
}