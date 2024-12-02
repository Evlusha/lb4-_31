
#include <iostream>         // Подключение библиотеки для ввода-вывода
#include <thread>           // Подключение библиотеки для работы с потоками
#include <vector>           // Подключение библиотеки для работы с векторами
#include <mutex>            // Подключение библиотеки для мьютексов
#include <atomic>           // Подключение библиотеки для атомарных операций
#include <condition_variable> // Подключение библиотеки для условных переменных
#include <chrono>           // Подключение библиотеки для работы с временем
#include <random>           // Подключение библиотеки для генерации случайных чисел
#include <sstream>          // Подключение библиотеки для работы с потоками строк

using namespace std;        // Использование стандартного пространства имен

constexpr int NUM_THREADS = 5;     // Константа, определяющая количество потоков
constexpr int NUM_ITERATIONS = 5;  // Константа, определяющая количество итераций в каждом потоке

mutex mtx;                    // Мьютекс для синхронизации потоков при выводе
condition_variable cv;        // Условная переменная для синхронизации потоков
atomic<bool> ready(false);    // Атомарная переменная для синхронизации потоков (используется для мониторинга)
atomic_flag spin_lock = ATOMIC_FLAG_INIT;  // Атомарный флаг для спинлоков

// Класс Barrier для реализации барьера для синхронизации потоков
class Barrier {
public:
    explicit Barrier(size_t count)  // Конструктор класса, инициализирует количество потоков
        : thread_count(count), waiting(0), generation(0) {}

    // Метод для потоков, которые приходят и ожидают на барьере
    void arrive_and_wait() {
        unique_lock<mutex> lock(m);  // Блокируем мьютекс для защиты от параллельного доступа

        size_t current_generation = generation;  // Сохраняем текущую генерацию для проверки

        ++waiting;  // Увеличиваем количество ожидающих потоков

        if (waiting == thread_count) {  // Если все потоки пришли, сбрасываем счетчик и увеличиваем поколение
            waiting = 0;
            generation++;  // Увеличиваем поколение
            cv.notify_all();  // Оповещаем все потоки, что барьер открыт
        } else {
            // Если не все потоки пришли, ждем, пока поколение не изменится
            cv.wait(lock, [this, current_generation] { return current_generation != generation; });
        }
    }

private:
    mutex m;  // Мьютекс для защиты состояния барьера
    condition_variable cv;  // Условная переменная для синхронизации
    size_t thread_count;  // Количество потоков, ожидающих на барьере
    size_t waiting;       // Счетчик ожидающих потоков
    size_t generation;    // Индикатор поколения для обеспечения корректной синхронизации
};

// Создаем объект барьера для синхронизации NUM_THREADS потоков
Barrier sync_barrier(NUM_THREADS);

// Класс Semaphore для реализации семафора с фиксированным количеством доступных ресурсов
class Semaphore {
public:
    explicit Semaphore(int count) : count(count) {}  // Конструктор, инициализирующий количество доступных ресурсов

    // Метод для захвата ресурса (уменьшает count, если доступен)
    void acquire() {
        unique_lock<mutex> lock(m);  // Блокируем мьютекс
        cv.wait(lock, [&]() { return count > 0; });  // Ожидаем, пока ресурсы будут доступны
        --count;  // Захватываем ресурс
    }

    // Метод для освобождения ресурса (увеличивает count, сигнализируя о его доступности)
    void release() {
        unique_lock<mutex> lock(m);  // Блокируем мьютекс
        ++count;  // Освобождаем ресурс
        cv.notify_one();  // Оповещаем один из ожидающих потоков
    }

private:
    mutex m;  // Мьютекс для синхронизации потоков
    condition_variable cv;  // Условная переменная для синхронизации потоков
    int count;  // Количество доступных ресурсов
};

// Создаем семафор с тремя доступными ресурсами
Semaphore semaphore(3);

// Функция для генерации случайного символа и вывода с использованием различных примитивов синхронизации
void generateRandomChar(int id, const string& sync_method) {
    random_device rd;  // Источник случайных чисел
    mt19937 gen(rd());  // Генератор случайных чисел
    uniform_int_distribution<> dist(33, 126);  // Диапазон символов ASCII для печатных символов

    for (int i = 0; i < NUM_ITERATIONS; ++i) {  // Цикл по числу итераций
        char randomChar = static_cast<char>(dist(gen));  // Генерируем случайный символ
        std::stringstream ss;  // Поток для формирования строки вывода

        // Используем различные примитивы синхронизации в зависимости от параметра sync_method
        if (sync_method == "mutex") {
            lock_guard<mutex> lock(mtx);  // Блокируем мьютекс для обеспечения синхронности
            ss << "Итерация " << i << ": Поток " << id << " захватил Mutex и выводит: " << randomChar << endl;
            cout << ss.str();  // Выводим строку

        } else if (sync_method == "semaphore") {
            semaphore.acquire();  // Захватываем семафор
            ss << "Итерация " << i << ": Поток " << id << " захватил Semaphore и выводит: " << randomChar << endl;
            semaphore.release();  // Освобождаем семафор
            ss << "Итерация " << i << ": Поток " << id << " освободил Semaphore" << endl;
            cout << ss.str();  // Выводим строку

        } else if (sync_method == "spinlock") {
            // Спинлок: бесконечно проверяем флаг и захватываем его
            while (spin_lock.test_and_set(memory_order_acquire));
            ss << "Итерация " << i << ": Поток " << id << " захватил SpinLock и выводит: " << randomChar << endl;
            spin_lock.clear(memory_order_release);  // Освобождаем спинлок
            cout << ss.str();  // Выводим строку

        } else if (sync_method == "barrier") {
            ss << "Итерация " << i << ": Поток " << id << " перед барьером." << endl;
            cout << ss.str();
            sync_barrier.arrive_and_wait();  // Ожидаем на барьере
            ss.str("");  // Очистка буфера
            ss << "Итерация " << i << ": Поток " << id << " прошел барьер и выводит: " << randomChar << endl;
            cout << ss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Небольшая задержка для упорядоченного вывода

        } else if (sync_method == "monitor") {
            unique_lock<mutex> lock(mtx);  // Блокируем мьютекс
            cv.wait(lock, []() { return ready.load(); });  // Ожидаем, пока поток не станет готов
            ss << "Итерация " << i << ": Поток " << id << " [Monitor]: " << randomChar << endl;
            cout << ss.str();

        } else if (sync_method == "spinwait") {
            int spin = 0;
            while (spin++ < 1000);  // Вращение в пустую для имитации спинлока без атомарных операций
            ss << "Итерация " << i << ": Поток " << id << " [SpinWait]: " << randomChar << endl;
            cout << ss.str();
        }
    }
}

// Функция для запуска теста с указанным методом синхронизации
void benchmark(const string& sync_method) {
    auto start = chrono::high_resolution_clock::now();  // Время начала теста

    vector<thread> threads;  // Вектор для хранения потоков
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(thread(generateRandomChar, i, sync_method));  // Запускаем потоки
    }
    if (sync_method == "monitor") {
        this_thread::sleep_for(chrono::milliseconds(100));  // Даем время на подготовку потоков
        ready.store(true);  // Устанавливаем флаг готовности
        cv.notify_all();  // Оповещаем все потоки
        ready.store(false);  // Сбрасываем флаг готовности
    }

    for (auto& th : threads) {
        th.join();  // Ждем завершения всех потоков
    }

    auto end = chrono::high_resolution_clock::now();  // Время окончания теста
    chrono::duration<double> elapsed = end - start;  // Разница во времени
    cout << "Время выполнения для " << sync_method << ": " << elapsed.count() << " секунд" << endl;
}

int main() {
    cout << "Запуск тестов для разных примитивов синхронизации...\n";
    benchmark("mutex");     // Тест с использованием мьютекса
    benchmark("semaphore"); // Тест с использованием семафора
    benchmark("spinlock");  // Тест с использованием спинлока
    benchmark("barrier");   // Тест с использованием барьера
    benchmark("monitor");   // Тест с использованием мониторинга
    benchmark("spinwait");  // Тест с использованием спиннелания

    return 0;  // Завершение программы
}
