

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <string>

using namespace std;



// Структура для описания нечёткого терма (треугольная функция принадлежности)
struct FuzzyTerm {
    string name;          // Название терма (например, "Близко", "Далеко")
    double left;          // Левая граница треугольника
    double peak;          // Вершина треугольника (максимальная принадлежность = 1)
    double right;         // Правая граница треугольника

    // Конструктор для удобного создания термов
    FuzzyTerm(string n, double l, double p, double r)
        : name(n), left(l), peak(p), right(r) {}
};

// Структура для правила нечёткого вывода
struct Rule {
    int inputTerm1Index;  // Индекс первого входного терма
    int inputTerm2Index;  // Индекс второго входного терма
    string logicOperator; // Логическая связка: "AND" или "OR"
    int outputTermIndex;  // Индекс выходного терма
    double weight;        // Вес правила (по умолчанию 1.0)

    Rule(int i1, int i2, string op, int out, double w = 1.0)
        : inputTerm1Index(i1), inputTerm2Index(i2), logicOperator(op),
        outputTermIndex(out), weight(w) {}
};

// Структура для хранения переменных системы
struct Variables {
    double distance;      // Вход: дистанция до цели (метры)
    double wind;          // Вход: сила ветра (м/с)
    double correction;    // Выход: поправка прицела (угловые минуты)
};

// ============================================================================
// ФУНКЦИОНАЛЬНЫЕ БЛОКИ
// ============================================================================

// Функция расчёта степени принадлежности для треугольной функции
// Возвращает значение от 0.0 до 1.0
double calculateMembership(double value, const FuzzyTerm& term) {
    // Если значение слева от треугольника
    if (value <= term.left || value >= term.right) {
        return 0.0;
    }
    // Если значение на левом склоне треугольника
    if (value >= term.left && value <= term.peak) {
        return (value - term.left) / (term.peak - term.left);
    }
    // Если значение на правом склоне треугольника
    if (value >= term.peak && value <= term.right) {
        return (term.right - value) / (term.right - term.peak);
    }
    return 0.0;
}

// Этап фаззификации: преобразование чётких входов в степени принадлежности
// Заполняет векторы membershipDistance и membershipWind
void fuzzify(const Variables& inputs,
    const vector<FuzzyTerm>& distanceTerms,
    const vector<FuzzyTerm>& windTerms,
    vector<double>& membershipDistance,
    vector<double>& membershipWind) {

    // Вычисляем степени принадлежности для дистанции
    membershipDistance.clear();
    for (const auto& term : distanceTerms) {
        membershipDistance.push_back(calculateMembership(inputs.distance, term));
    }

    // Вычисляем степени принадлежности для ветра
    membershipWind.clear();
    for (const auto& term : windTerms) {
        membershipWind.push_back(calculateMembership(inputs.wind, term));
    }
}

// Этап логического вывода: активация правил и агрегирование
// Возвращает вектор активированных значений для выходных термов
vector<double> infer(const vector<Rule>& rules,
    const vector<double>& membershipDistance,
    const vector<double>& membershipWind,
    int outputTermsCount) {

    // Инициализируем вектор активированных значений нулями
    vector<double> activatedValues(outputTermsCount, 0.0);

    // Проходим по всем правилам базы знаний
    for (const auto& rule : rules) {
        double activationLevel = 0.0;

        // Вычисляем уровень активации правила
        if (rule.logicOperator == "AND") {
            // Операция min для логического "И"
            activationLevel = min(membershipDistance[rule.inputTerm1Index],
                membershipWind[rule.inputTerm2Index]);
        }
        else if (rule.logicOperator == "OR") {
            // Операция max для логического "ИЛИ"
            activationLevel = max(membershipDistance[rule.inputTerm1Index],
                membershipWind[rule.inputTerm2Index]);
        }

        // Применяем вес правила
        activationLevel *= rule.weight;

        // Агрегирование: выбираем максимум для каждого выходного терма
        if (activationLevel > activatedValues[rule.outputTermIndex]) {
            activatedValues[rule.outputTermIndex] = activationLevel;
        }
    }

    return activatedValues;
}

// Этап дефазификации: преобразование нечёткого результата в чёткое число
// Использует метод центра тяжести (centroid method)
double defuzzify(const vector<double>& activatedValues,
    const vector<FuzzyTerm>& outputTerms) {

    double numerator = 0.0;    // Числитель формулы центра тяжести
    double denominator = 0.0;  // Знаменатель формулы центра тяжести

    // Проходим по всем выходным термам
    for (size_t i = 0; i < outputTerms.size(); i++) {
        // Используем вершину треугольника как представительное значение
        double representativeValue = outputTerms[i].peak;
        double activationLevel = activatedValues[i];

        // Накопление взвешенных значений
        numerator += representativeValue * activationLevel;
        denominator += activationLevel;
    }

    // Защита от деления на ноль
    if (denominator == 0.0) {
        return 0.0;
    }

    // Возвращаем чёткое значение поправки
    return numerator / denominator;
}

// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================================

int main() {

    setlocale(LC_ALL, "Russian");

    // Настройка формата вывода чисел
    cout << fixed << setprecision(2);

    cout << "================================================================" << endl;
    cout << "   СИСТЕМА НЕЧЁТКОГО ВЫВОДА ДЛЯ БАЛЛИСТИЧЕСКИХ РАСЧЁТОВ" << endl;
    cout << "================================================================" << endl;
    cout << endl;

    // =========================================================================
    // ИНИЦИАЛИЗАЦИЯ БАЗЫ ТЕРМОВ
    // =========================================================================

    // Входная переменная: Дистанция до цели (метры)
    vector<FuzzyTerm> distanceTerms = {
        FuzzyTerm("Близко",    0,   0,   300),    // 0-300 м
        FuzzyTerm("Средне",   200, 500,  800),    // 200-800 м
        FuzzyTerm("Далеко",   700, 1000, 1300)    // 700-1300 м
    };

    // Входная переменная: Сила ветра (м/с)
    vector<FuzzyTerm> windTerms = {
        FuzzyTerm("Слабый",   0, 0, 5),     // 0-5 м/с
        FuzzyTerm("Умеренный", 3, 6, 9),    // 3-9 м/с
        FuzzyTerm("Сильный",  7, 12, 17)    // 7-17 м/с
    };

    // Выходная переменная: Поправка прицела (угловые минуты)
    vector<FuzzyTerm> correctionTerms = {
        FuzzyTerm("Минимальная", 0, 0, 2),     // 0-2 угл. мин
        FuzzyTerm("Средняя",    1, 3, 5),     // 1-5 угл. мин
        FuzzyTerm("Максимальная", 4, 7, 10)   // 4-10 угл. мин
    };

    // =========================================================================
    // ИНИЦИАЛИЗАЦИЯ БАЗЫ ПРАВИЛ
    // =========================================================================

    vector<Rule> rules = {
        // Правило 1: Если близко И ветер слабый → поправка минимальная
        Rule(0, 0, "AND", 0, 1.0),

        // Правило 2: Если близко И ветер умеренный → поправка минимальная
        Rule(0, 1, "AND", 0, 1.0),

        // Правило 3: Если средне И ветер слабый → поправка средняя
        Rule(1, 0, "AND", 1, 1.0),

        // Правило 4: Если средне И ветер умеренный → поправка средняя
        Rule(1, 1, "AND", 1, 1.0),

        // Правило 5: Если далеко И ветер сильный → поправка максимальная
        Rule(2, 2, "AND", 2, 1.0),

        // Правило 6: Если далеко ИЛИ ветер сильный → поправка максимальная
        Rule(2, 2, "OR", 2, 1.0),

        // Правило 7: Если близко И ветер сильный → поправка средняя
        Rule(0, 2, "AND", 1, 1.0),

        // Правило 8: Если далеко И ветер слабый → поправка средняя
        Rule(2, 0, "AND", 1, 1.0)
    };

    cout << "База термов инициализирована:" << endl;
    cout << "  Дистанция: " << distanceTerms.size() << " термов" << endl;
    cout << "  Ветер: " << windTerms.size() << " термов" << endl;
    cout << "  Поправка: " << correctionTerms.size() << " термов" << endl;
    cout << "  Правила: " << rules.size() << " правил" << endl;
    cout << endl;

    // =========================================================================
    // ЦИКЛ ВВОДА И ОБРАБОТКИ
    // =========================================================================

    char continueCalculation = 'y';

    while (continueCalculation == 'y' || continueCalculation == 'Y') {
        Variables inputs;

        // Запрос входных данных у пользователя
        cout << "----------------------------------------------------------------" << endl;
        cout << "Введите дистанцию до цели (метры, 0-1300): ";
        cin >> inputs.distance;

        cout << "Введите силу ветра (м/с, 0-17): ";
        cin >> inputs.wind;

        // Проверка корректности ввода
        if (inputs.distance < 0 || inputs.distance > 1300 ||
            inputs.wind < 0 || inputs.wind > 17) {
            cout << "Ошибка: значения вне допустимого диапазона!" << endl;
            continue;
        }

        cout << endl;
        cout << "Обработка данных..." << endl;
        cout << endl;

        // =========================================================================
        // ЭТАП 1: ФАЗЗИФИКАЦИЯ
        // =========================================================================

        vector<double> membershipDistance;
        vector<double> membershipWind;

        fuzzify(inputs, distanceTerms, windTerms, membershipDistance, membershipWind);

        cout << "Этап 1: Фаззификация" << endl;
        cout << "  Степени принадлежности для дистанции:" << endl;
        for (size_t i = 0; i < distanceTerms.size(); i++) {
            cout << "    " << distanceTerms[i].name << ": "
                << membershipDistance[i] << endl;
        }

        cout << "  Степени принадлежности для ветра:" << endl;
        for (size_t i = 0; i < windTerms.size(); i++) {
            cout << "    " << windTerms[i].name << ": "
                << membershipWind[i] << endl;
        }
        cout << endl;

        // =========================================================================
        // ЭТАП 2: ЛОГИЧЕСКИЙ ВЫВОД
        // =========================================================================

        vector<double> activatedValues = infer(rules, membershipDistance,
            membershipWind,
            correctionTerms.size());

        cout << "Этап 2: Логический вывод (активация правил)" << endl;
        cout << "  Активированные значения выходных термов:" << endl;
        for (size_t i = 0; i < correctionTerms.size(); i++) {
            cout << "    " << correctionTerms[i].name << ": "
                << activatedValues[i] << endl;
        }
        cout << endl;

        // =========================================================================
        // ЭТАП 3: ДЕФАЗЗИФИКАЦИЯ
        // =========================================================================

        inputs.correction = defuzzify(activatedValues, correctionTerms);

        cout << "Этап 3: Дефаззификация (метод центра тяжести)" << endl;
        cout << "  Итоговая поправка прицела: +" << inputs.correction
            << " угловых минут" << endl;
        cout << endl;

        // =========================================================================
        // ВЫВОД РЕЗУЛЬТАТА
        // =========================================================================

        cout << "================================================================" << endl;
        cout << "РЕЗУЛЬТАТ РАСЧЁТА" << endl;
        cout << "================================================================" << endl;
        cout << "Дистанция: " << inputs.distance << " м" << endl;
        cout << "Ветер: " << inputs.wind << " м/с" << endl;
        cout << "ПОПРАВКА ПРИЦЕЛА: +" << inputs.correction << " угл. мин" << endl;
        cout << "================================================================" << endl;
        cout << endl;

        // Запрос на продолжение работы
        cout << "Выполнить ещё один расчёт? (y/n): ";
        cin >> continueCalculation;
        cout << endl;
    }

    // =========================================================================
    // ЗАВЕРШЕНИЕ РАБОТЫ
    // =========================================================================

    cout << "================================================================" << endl;
    cout << "Работа программы завершена." << endl;
    cout << "================================================================" << endl;

    return 0;
}