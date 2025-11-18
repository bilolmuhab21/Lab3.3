#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <windows.h>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

class PayrollException : public runtime_error {
public:
    explicit PayrollException(const string& msg)
        : runtime_error(msg) {}
};

class InvalidRateException : public PayrollException {
public:
    explicit InvalidRateException(const string& msg)
        : PayrollException("Invalid rate: " + msg) {}
};

class DuplicateWorkTypeException : public PayrollException {
public:
    explicit DuplicateWorkTypeException(const string& msg)
        : PayrollException("Duplicate work type: " + msg) {}
};

class EmptyWorkListException : public PayrollException {
public:
    explicit EmptyWorkListException(const string& msg)
        : PayrollException("Work list is empty: " + msg) {}
};


class IBonusStrategy {
public:
    virtual ~IBonusStrategy() = default;
    virtual double computePay(double basePay) const = 0;
};

class NoBonusStrategy : public IBonusStrategy {
public:
    double computePay(double basePay) const override {
        return basePay;
    }
};

class PercentageBonusStrategy : public IBonusStrategy {
private:
    double bonusPercent;

public:
    explicit PercentageBonusStrategy(double percent)
        : bonusPercent(percent)
    {
        if (bonusPercent < 0) {
            throw InvalidRateException("bonus percent must be >= 0");
        }
    }

    double computePay(double basePay) const override {
        return basePay * (1.0 + bonusPercent / 100.0);
    }
};


class IWorkType {
public:
    virtual ~IWorkType() = default;

    virtual string getName() const = 0;
    virtual double getBasePay() const = 0;
    virtual double getFinalPay() const = 0;
};

class WorkTypeBase : public IWorkType {
private:
    string name;
    double basePay;
    shared_ptr<IBonusStrategy> bonusStrategy;

public:
    WorkTypeBase(const string& name,
        double basePay,
        shared_ptr<IBonusStrategy> strategy)
        : name(name), basePay(basePay), bonusStrategy(strategy)
    {
        if (name.empty()) {
            throw InvalidRateException("work type name must not be empty");
        }
        if (basePay <= 0) {
            throw InvalidRateException("base pay must be > 0");
        }
        if (!bonusStrategy) {
            throw InvalidRateException("bonus strategy must not be null");
        }
    }

    string getName() const override {
        return name;
    }

    double getBasePay() const override {
        return basePay;
    }

    double getFinalPay() const override {
        return bonusStrategy->computePay(basePay);
    }
};


class PayrollDepartment {
private:
    vector<shared_ptr<IWorkType>> workTypes;

    bool existsWorkType(const string& name) const {
        for (const auto& w : workTypes) {
            if (w->getName() == name) {
                return true;
            }
        }
        return false;
    }

public:
    PayrollDepartment() = default;

    void addWorkType(const string& name,
        double basePay,
        double bonusPercent = 0.0)
    {
        if (name.size() > 50) {
            cerr << "Предупреждение: название типа работ очень длинное\n";
        }

        if (existsWorkType(name)) {
            throw DuplicateWorkTypeException(
                "work type '" + name + "' already exists");
        }

        shared_ptr<IBonusStrategy> strategy;

        if (bonusPercent == 0.0) {
            strategy = make_shared<NoBonusStrategy>();
        }
        else {
            strategy = make_shared<PercentageBonusStrategy>(bonusPercent);
        }

        auto wt = make_shared<WorkTypeBase>(name, basePay, strategy);
        workTypes.push_back(wt);
    }

    double calculateAveragePay() const {
        if (workTypes.empty()) {
            throw EmptyWorkListException("cannot calculate average");
        }

        double sum = 0.0;
        for (const auto& w : workTypes) {
            sum += w->getFinalPay();
        }
        return sum / static_cast<double>(workTypes.size());
    }

    void printAll() const {
        if (workTypes.empty()) {
            cout << "Список типов работ пуст.\n";
            return;
        }

        cout << "Текущие типы работ:\n";
        for (const auto& w : workTypes) {
            cout << "  - " << w->getName()
                << " | базовая оплата: " << w->getBasePay()
                << " | с надбавкой: " << w->getFinalPay()
                << '\n';
        }
    }
};


void clearStdin() {
    cin.clear();
    cin.ignore(32767, '\n');
}

string inputNonEmptyString(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);

        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == string::npos || end == string::npos) {
            cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
            continue;
        }
        s = s.substr(start, end - start + 1);

        if (!s.empty()) return s;
        cout << "Ошибка: строка не может быть пустой. Попробуйте снова.\n";
    }
}

double inputPositiveDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double x;
        if (!(cin >> x)) {
            cout << "Ошибка: введите число.\n";
            clearStdin();
            continue;
        }
        clearStdin();
        if (x <= 0.0) {
            cout << "Ошибка: значение должно быть больше 0. Попробуйте снова.\n";
            continue;
        }
        if (x > 1000000.0) {
            cout << "Ошибка: значение не должно превышать 1000000. Попробуйте снова.\n";
            continue;
        }
        return x;
    }
}

double inputNonNegativeDouble(const string& prompt) {
    while (true) {
        cout << prompt;
        double x;
        if (!(cin >> x)) {
            cout << "Ошибка: введите число.\n";
            clearStdin();
            continue;
        }
        clearStdin();
        if (x >= 0.0) return x;
        cout << "Ошибка: значение не может быть отрицательным. Попробуйте снова.\n";
    }
}

int inputMenuChoice(const string& prompt, int low, int high) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) {
            cin.clear();
            continue;
        }

        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == string::npos || end == string::npos) {
            cout << "Ошибка: введите число.\n";
            continue;
        }
        string s = line.substr(start, end - start + 1);

        bool allDigits = !s.empty() &&
            all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c); });

        if (!allDigits) {
            cout << "Ошибка: введите целое число.\n";
            continue;
        }

        int val = stoi(s);
        if (val < low || val > high) {
            cout << "Ошибка: число должно быть в диапазоне от 0 до 3.\n";
            continue;
        }
        return val;
    }
}


int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    setlocale(LC_ALL, "Russian");

    PayrollDepartment dept;

    while (true) {
        cout << "\n===== МЕНЮ ОТДЕЛА РАСЧЁТА ЗАРПЛАТЫ =====\n";
        cout << "1. Добавить тип работ\n";
        cout << "2. Показать все типы работ\n";
        cout << "3. Вычислить среднюю величину оплаты\n";
        cout << "0. Выход\n";
        cout << "========================================\n";

        int choice = inputMenuChoice("Ваш выбор: ", 0, 3);

        try {
            if (choice == 0) {
                cout << "Выход из программы.\n";
                break;
            }
            else if (choice == 1) {
                string name = inputNonEmptyString("Введите название типа работ: ");
                double basePay = inputPositiveDouble("Введите базовую оплату: ");
                double bonusPercent = inputNonNegativeDouble("Введите надбавку в процентах (0 если нет): ");

                dept.addWorkType(name, basePay, bonusPercent);
                cout << "Тип работ успешно добавлен.\n";

            }
            else if (choice == 2) {
                dept.printAll();
            }
            else if (choice == 3) {
                double avg = dept.calculateAveragePay();
                cout << fixed << setprecision(2);
                cout << "Средняя величина оплаты: " << avg << '\n';
            }
        }
        catch (const PayrollException& ex) {
            cout << "Ошибка расчёта зарплаты: " << ex.what() << '\n';
        }
        catch (const exception& ex) {
            cout << "Непредвиденная ошибка: " << ex.what() << '\n';
        }
    }

    return 0;
}
