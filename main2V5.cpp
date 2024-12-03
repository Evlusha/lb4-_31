//VAR5
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <random>
#include <numeric>

using namespace std;

struct ExamResult {
    int semester; // Номер семестра
    string subject; // Название дисциплины
    int grade; // Оценка за экзамен (от 2 до 5)
};

struct Student {
    string fullName; // ФИО
    string groupNumber; // Номер группы
    vector<ExamResult> results; // Список результатов сессий
};

// Глобальная переменная для безопасного доступа к данным анализа
mutex mtx;

// Функция для вычисления средней успеваемости студентами в однопоточном режиме
double calculateAverageSingleThread(const vector<Student>& students, const string& group, int semester) {
    double sumGrades = 0.0; // Сумма оценок
    int count = 0; //Количество оценок
    for (const auto& student : students) { //Прохожусь по всем студентам
        if (student.groupNumber == group) { // проверка на принадлежность студента к определенной группе
            for (const auto& result : student.results) {
                if (result.semester == semester) {///Проверяем соответствует ли семестр для текущего результата запрашиваемому семестру
                    sumGrades += result.grade;//
                    count++;//
                }
            }
        }
    }
    return count == 0 ? 0.0 : (sumGrades / count);
}

// Функция для многопоточного вычисления средней успеваемости
void calculateAverageForPart(const vector<Student>& students, const string& group, int semester, int start, int end, double& partialSum, int& partialCount) {
    double sumGrades = 0.0;
    int count = 0;
    
    for (int i = start; i < end; ++i) {
        if (students[i].groupNumber == group) {
            for (const auto& result : students[i].results) {
                if (result.semester == semester) {
                    sumGrades += result.grade;
                    count++;
                }
            }
        }
    }

    lock_guard<mutex> guard(mtx);
    partialSum += sumGrades;
    partialCount += count;
}

// Функция для генерации случайных данных
Student generateRandomStudent(int studentId) {
    static const vector<string> names = {"Алиса", "Дима", "Илья", "Карась", "Васек"};
    static const vector<string> subjects = {"Матан", "Философия", "Геометрия", "Биология", "История"};
    static string groupName = "G1";
    static mt19937 rng(random_device{}());
    uniform_int_distribution<> gradeDist(2, 5);
    uniform_int_distribution<> semDist(1, 8);
    uniform_int_distribution<> nameDist(0, names.size() - 1);
    uniform_int_distribution<> subjDist(0, subjects.size() - 1);

    Student student;
    student.fullName = names[nameDist(rng)] + " " + to_string(studentId);
    student.groupNumber = groupName;
    
    int numberOfResults = 5;  // Например, 5 предметов
    for (int i = 0; i < numberOfResults; ++i) {
        ExamResult result;
        result.semester = semDist(rng);
        result.subject = subjects[subjDist(rng)];
        result.grade = gradeDist(rng);
        student.results.push_back(result);
    }
    return student;
}

int main() {
    int numberOfStudents;
    int numberOfThreads;
    string group = "G1";
    int semester = 1;

    cout << "Введите количество студентов: ";
    cin >> numberOfStudents;
    cout << "Введите количество потоков: ";
    cin >> numberOfThreads;
    
    vector<Student> students(numberOfStudents);

    char fillChoice;
    cout << "Заполнить данные вручную (введите 'm') или случайно (введите 'r')? ";
    cin >> fillChoice;

    if (fillChoice == 'm') {
        for (int i = 0; i < numberOfStudents; ++i) {
            cout << "Введите имя студента #" << i+1 << ": ";
            cin >> students[i].fullName;
            cout << "Введите номер группы студента #" << i+1 << ": ";
            cin >> students[i].groupNumber;
            cout << "Введите количество результатов сессий для студента #" << i+1 << ": ";
            int numResults;
            cin >> numResults;
            students[i].results.resize(numResults);
            for (int j = 0; j < numResults; ++j) {
                cout << "Введите номер семестра: ";
                cin >> students[i].results[j].semester;
                cout << "Введите название дисциплины: ";
                cin >> students[i].results[j].subject;
                cout << "Введите оценку за экзамен: ";
                cin >> students[i].results[j].grade;
            }
        }
    } else if (fillChoice == 'r') {
        for (int i = 0; i < numberOfStudents; ++i) {
            students[i] = generateRandomStudent(i+1);
        }
    }

    // Однопоточное вычисление
    auto start = chrono::high_resolution_clock::now();
    double averageSingle = calculateAverageSingleThread(students, group, semester);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> durationSingle = end - start;
    cout << "Средняя успеваемость в однопоточном режиме: " << averageSingle << " (заняло " << durationSingle.count() << " сек)" << endl;

    // Многопоточное вычисление
    double sumParallel = 0.0;
    int countParallel = 0;
    int chunkSize = numberOfStudents / numberOfThreads;
    vector<thread> threads;

    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < numberOfThreads; ++i) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numberOfThreads - 1) ? numberOfStudents : startIdx + chunkSize;
        threads.emplace_back(calculateAverageForPart, ref(students), group, semester, startIdx, endIdx, ref(sumParallel), ref(countParallel));
    }

    for (auto& th : threads) {
        th.join();
    }
    double averageParallel = countParallel == 0 ? 0.0 : (sumParallel / countParallel);
    end = chrono::high_resolution_clock::now();
    chrono::duration<double> durationParallel = end - start;
    cout << "Средняя успеваемость в многопоточном режиме: " << averageParallel << " (заняло " << durationParallel.count() << " сек)" << endl;

    return 0;
}
