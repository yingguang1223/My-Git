#include <iostream>
#include <cstdlib>  
#include "Goods.h"
#include "DerivedGoods.h"
#include "Operations.h"
using namespace std;


Goods* inventory[1000];   
int inventoryCount = 0;   


void loadAllData();
void saveAllData();

int main() {
   
    system("mkdir .\\data");  

    
    
    try {
        loadAllData();   
    } catch (const exception& e) {
        cerr << "加载数据时出错（可能首次运行无文件），继续运行：" << e.what() << endl;
    }

    int choice;
    while (true) {
        system("cls"); 
        cout << "\n===== 超市管理系统 =====" << endl;
        cout << "1. 增加商品\n2. 出售商品\n3. 查询商品（关键词）\n4. 显示全部\n5. 修改商品单价\n6. 按价格区间查询\n0. 退出\n";
        cout << "请选择: ";
        cin >> choice;

        if (choice == 0) break;

       
        try {
            switch (choice) {
                case 1: addItem(); break;      
                case 2: sellItem(); break;     
                case 3: searchItem(); break;   
                case 4: showAll(); break;      
                case 5: updatePrice(); break;  
                case 6: searchByPriceRange(); break;  
                default: cout << "无效选项，请重试" << endl;
            }
        } catch (const exception& e) {
            cerr << "操作异常：" << e.what() << endl;
        }
        system("pause");
    }


    
try {
    saveAllData();
    cout << "数据保存成功！" << endl;
} catch (const exception& e) {
    cerr << "保存数据时出错：" << e.what() << endl;
    cerr << "请检查文件夹权限，但程序仍会安全退出。" << endl;
}


for (int i = 0; i < inventoryCount; i++) {
    delete inventory[i];
}
inventoryCount = 0;
cout << "内存已释放，安全退出！" << endl;
return 0;
}