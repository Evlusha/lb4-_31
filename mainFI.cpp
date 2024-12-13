//fill
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

class MyVector {
public: // можно использовать в любой точке кода
    MyVector() : size_(0), capacity_(1) { // конструктор - инициализация
        data_ = new thread * [capacity_];
    }

    ~MyVector() { //деструктор - освобожденине памяти
        delete[] data_;
    }

    void push_back(thread* t) { //добавление в конец массива
        if (size_ >= capacity_) {
            resize();
        }
        data_[size_++] = t;
    }

    thread* operator[](size_t index) { //доступ к элементу
        return data_[index];
    }

    size_t size() const { //узнать размер массива
        return size_;
    }

private: // использование только внутри класса
    thread** data_; //указатель на массив указателей (потоки)
    size_t size_; //размер
    size_t capacity_;//емкость

    void resize() { // метод увеличение емкости массива
        capacity_ *= 2;
        thread** new_data = new thread * [capacity_];
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[i];
        }
        delete[] data_;
        data_ = new_data;
    }
};

const int num_philosophers = 5; // количество философов

mutex forks[num_philosophers]; // Вилки между философами
mutex output_mutex; // Мьютекс для синхронизации вывода

void philosopher(int id) {
    while (true) {
        // Философ размышляет
        {
            lock_guard<mutex> lock(output_mutex); // захватываем мьютексом вывод в консоль
            cout << "Философ " << id << " размышляет.\n"; 
        }
        this_thread::sleep_for(chrono::milliseconds(5000));

        // Взятие вилок по порядку
        mutex& first_fork = forks[min(id, (id + 1) % num_philosophers)]; //сначала берем вилку с меньшим номером 
        mutex& second_fork = forks[max(id, (id + 1) % num_philosophers)]; // потом берем вилку с большим номером

        // Захват вилок
        lock(first_fork, second_fork); // Зафиксируем обе вилки
        lock_guard<mutex> lg1(first_fork, adopt_lock); // Зафиксировать первую вилку (передача через параметр adopt_lock уже ранее захваченного мьютекса)
        lock_guard<mutex> lg2(second_fork, adopt_lock); // Зафиксировать вторую вилку

        // Философ ест
        {
            lock_guard<mutex> lock(output_mutex); // забираем мьютексом вывод
            cout << "Философ " << id << " начал есть.\n"; // Изменено сообщение
        }
        this_thread::sleep_for(chrono::milliseconds(5000));

        // Философ освобождает вилки (выйдет из области видимости, и вилки будут освобождены)
        {
            lock_guard<mutex> lock(output_mutex); // забираем мьютексом вывод
            cout << "Философ " << id << " освободил вилки.\n"; // Новое сообщение о освобождении вилок
        }
    }
}

int main() {
    MyVector philosophers; // вектор с философами 

    for (int i = 0; i < num_philosophers; ++i) {
        philosophers.push_back(new thread(philosopher, i)); // создаем поток(философа) и заплняяем вектор
    }

    for (size_t i = 0; i < philosophers.size(); ++i) { //запускаем потоки
        philosophers[i]->join(); 
        delete philosophers[i]; // освобождаем память
    }

    return 0;
}
