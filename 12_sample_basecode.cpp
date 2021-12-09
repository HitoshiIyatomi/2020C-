#include <iostream>

using namespace std;

class vehicle {
    int num_wheels;
    int range;
public:
    vehicle(int w, int r) {
        num_wheels = w;
        range = r;
    }
    void show() {
        cout << "num wheels is: " << num_wheels << endl;
        cout << "range is: " << range << endl;
    }
};

class car : public vehicle {
    int passengers;
public:
    car(int w, int r, int p) : vehicle(w, r) {
        passengers = p;
    }
    void show() {
        cout << "\n car" << endl;
        vehicle::show();
        cout << "This is CAR: maximum passengers is " << passengers << endl;
    }
};


int main()
{
    vehicle* v[2];
    vehicle my_vehicle(6, 1000);
    car my_car(4, 800, 5);

    v[0] = &my_vehicle;
    v[1] = &my_car;

    for (int i = 0; i < 2; i++)
        v[i]->show();

    return 0;
}