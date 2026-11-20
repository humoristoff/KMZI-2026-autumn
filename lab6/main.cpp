#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <numeric>
#include <cassert>
#include <iomanip>
#include <algorithm>

// ============================================================================
// Лабораторная работа №6: Алгоритмы Дойча, Дойча-Йожи. Алгоритм Шора
// Вариант 7:
// - Таблица А: N = 35, a = 2
// ============================================================================

// НОД
long long gcd(long long a, long long b) {
    while (b) {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

// Быстрое возведение в степень по модулю
long long modPow(long long base, long long exp, long long mod) {
    long long res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (__int128(res) * base) % mod;
        base = (__int128(base) * base) % mod;
        exp /= 2;
    }
    return res;
}

// Функция Эйлера phi(n)
int eulerPhi(int n) {
    int res = n;
    for (int p = 2; p * p <= n; ++p) {
        if (n % p == 0) {
            while (n % p == 0) n /= p;
            res -= res / p;
        }
    }
    if (n > 1) res -= res / n;
    return res;
}

// --- Задание 1: Алгоритм Дойча ---
struct DeutschOracle {
    int type; // 1: f=0, 2: f=x, 3: f=!x, 4: f=1
    int eval(int x) const {
        if (type == 1) return 0;
        if (type == 2) return x;
        if (type == 3) return 1 - x;
        return 1;
    }
    bool isConstant() const {
        return (type == 1 || type == 4);
    }
};

bool runDeutschAlgorithm(const DeutschOracle& oracle) {
    // 2 кубита: q0 - входной/управляющий, q1 - целевой (|1>)
    // 1. Применение H к обоим: q0 в |+>, q1 в |->
    // 2. Оракул U_f: фазовый сдвиг (-1)^{f(x)}
    // 3. Применение H к q0
    // Если функция постоянная, q0 переходит в |0>
    // Если функция сбалансированная, q0 переходит в |1>
    int f0 = oracle.eval(0);
    int f1 = oracle.eval(1);

    // Вектор состояния кубита 0 после H, оракула и финального H:
    // Амплитуда |0>: ( (-1)^f0 + (-1)^f1 ) / 2
    // Амплитуда |1>: ( (-1)^f0 - (-1)^f1 ) / 2
    double amp0 = (std::pow(-1.0, f0) + std::pow(-1.0, f1)) / 2.0;
    double prob0 = amp0 * amp0;

    // Измерение: 0 означает постоянную, 1 - сбалансированную
    return (prob0 > 0.99); // true = постоянная
}

void testDeutsch() {
    std::cout << "[Задание 1] Тестирование алгоритма Дойча:\n";
    std::cout << " Оракул | f(0) | f(1) | Тип функции   | Результат алгоритма | Статус\n";
    std::cout << "--------+------+------+---------------+---------------------+--------\n";
    for (int type = 1; type <= 4; ++type) {
        DeutschOracle oracle{type};
        bool isConst = runDeutschAlgorithm(oracle);
        assert(isConst == oracle.isConstant());
        std::cout << " f_" << type << "    |  " << oracle.eval(0) << "   |  "
                  << oracle.eval(1) << "   | "
                  << (oracle.isConstant() ? "Постоянная   " : "Сбалансиров. ") << "| "
                  << (isConst ? "Постоянная       " : "Сбалансированная ") << "| OK\n";
    }
    std::cout << "[+] Алгоритм Дойча успешно распознал все 4 функции с 1 вызова оракула!\n\n";
}

// --- Задание 2: Алгоритм Дойча-Йожи ---
void testDeutschJozsa() {
    std::cout << "[Задание 2] Алгоритм Дойча-Йожи (n = 3 кубита):\n";
    // Для n кубитов: f: {0,1}^n -> {0,1}
    // Постоянная функция f(x) = 1
    // Амплитуда |000> равна 1/2^n * sum_x (-1)^f(x)
    int n = 3;
    int N = 1 << n; // 8

    // Тест постоянной функции f(x) = 1
    double sumConst = 0;
    for (int x = 0; x < N; ++x) sumConst += std::pow(-1.0, 1);
    double prob0_const = (sumConst / N) * (sumConst / N);
    assert(std::abs(prob0_const - 1.0) < 1e-6);

    // Тест сбалансированной функции (f(x) = x & 1)
    double sumBalanced = 0;
    for (int x = 0; x < N; ++x) sumBalanced += std::pow(-1.0, x & 1);
    double prob0_bal = (sumBalanced / N) * (sumBalanced / N);
    assert(std::abs(prob0_bal - 0.0) < 1e-6);

    std::cout << "  - Постоянная функция: P(|0...0>) = " << prob0_const << " (100% распознавание)\n";
    std::cout << "  - Сбалансированная:  P(|0...0>) = " << prob0_bal << " (0% попадания в |0...0>)\n";
    std::cout << "[+] Алгоритм Дойча-Йожи подтверждён!\n\n";
}

// --- Задания 3-6: Алгоритм Шора (Вариант 7) ---
// Непрерывная дробь для нахождения порядка r из измеренной фазы y / 2^m
long long findOrderFromPhase(long long y, long long two_m, long long N, long long a) {
    // Разложение y / 2^m в непрерывную дробь
    long long p0 = 0, p1 = 1;
    long long q0 = 1, q1 = 0;
    long long num = y, den = two_m;

    while (den != 0) {
        long long q = num / den;
        long long rem = num % den;

        long long p2 = q * p1 + p0;
        long long q2 = q * q1 + q0;

        if (q2 >= N) break;

        // Кандидат r = q2
        if (q2 > 1 && modPow(a, q2, N) == 1) {
            return q2;
        }

        p0 = p1; p1 = p2;
        q0 = q1; q1 = q2;
        num = den;
        den = rem;
    }
    return -1;
}

void testShorVariant7() {
    const long long N = 35;
    const long long a = 2;

    std::cout << "[Задание 3-6] Алгоритм Шора (Вариант 7: Таблица А):\n";
    std::cout << "  Составное число N = " << N << ", Основание a = " << a << "\n";

    // 1. Проверка gcd(a, N) = 1
    long long g = gcd(a, N);
    std::cout << "  1. Проверка взаимной простоты: gcd(" << a << ", " << N << ") = " << g << "\n";
    assert(g == 1);

    // 2. Классический поиск порядка r для проверки
    long long true_r = 1;
    while (modPow(a, true_r, N) != 1) {
        true_r++;
    }
    std::cout << "  2. Порядок числа a = 2 по модулю 35: r = " << true_r << "\n";
    assert(true_r == 12);

    // 3. Параметры квантовой схемы
    int n = (int)std::ceil(std::log2(N)); // n = 6
    int m = 2 * n;                         // m = 12
    long long two_m = 1LL << m;           // 4096

    std::cout << "  3. Параметры квантовой схемы:\n";
    std::cout << "     - Целевой регистр: n = ceil(log2(" << N << ")) = " << n << " кубитов\n";
    std::cout << "     - Управляющий регистр: m = 2n = " << m << " кубитов (размерность 2^m = " << two_m << ")\n";

    // 4. Моделирование квантового измерения фазы
    std::cout << "\n  4. Моделирование фазовой оценки и квантовых измерений:\n";
    std::cout << "   s | Фаза s/r | Дискретное y | Непрерывная дробь (r) | Статус кандидата\n";
    std::cout << "  ---+----------+--------------+-----------------------+------------------\n";

    int successfulMeasurements = 0;
    for (int s = 0; s < true_r; ++s) {
        double phase = (double)s / true_r;
        long long y = std::llround(phase * two_m);
        long long cand_r = findOrderFromPhase(y, two_m, N, a);

        bool success = (cand_r == true_r);
        if (success) successfulMeasurements++;

        std::cout << "  " << std::setw(2) << s << " |  "
                  << std::fixed << std::setprecision(4) << phase << "  |     "
                  << std::setw(4) << y << "     |           "
                  << (cand_r > 0 ? std::to_string(cand_r) : "-") << "          | "
                  << (success ? "Засчитано (r=12)" : "Делитель r (или 0)") << "\n";
    }

    double successRate = (double)successfulMeasurements / true_r;
    double theorRate = (double)eulerPhi((int)true_r) / true_r; // phi(12)/12 = 4/12 = 0.3333

    std::cout << "\n  Доля успешных измерений порядка r: " << successRate * 100.0 << "%\n";
    std::cout << "  Теоретическая величина phi(r)/r:   " << theorRate * 100.0 << "%\n";
    assert(std::abs(successRate - theorRate) < 1e-4);
    std::cout << "  [+] Доля засчитанных измерений в точности равна phi(r)/r!\n\n";

    // 5. Вычисление нетривиальных множителей
    std::cout << "  5. Факторизация числа N = " << N << " по найденному порядку r = 12:\n";
    assert(true_r % 2 == 0); // r чётно
    long long half_r = true_r / 2; // 6
    long long x = modPow(a, half_r, N); // 2^6 mod 35 = 64 mod 35 = 29
    std::cout << "     x = a^(r/2) mod N = 2^6 mod 35 = " << x << "\n";
    assert(x != N - 1 && x != 1);

    long long p = gcd(x - 1, N); // gcd(28, 35) = 7
    long long q = gcd(x + 1, N); // gcd(30, 35) = 5

    std::cout << "     d1 = gcd(x - 1, N) = gcd(" << x - 1 << ", " << N << ") = " << p << "\n";
    std::cout << "     d2 = gcd(x + 1, N) = gcd(" << x + 1 << ", " << N << ") = " << q << "\n";

    assert(p * q == N);
    assert(p > 1 && p < N);
    assert(q > 1 && q < N);

    std::cout << "\n  [+] УСПЕХ: Найдены нетривиальные множители числа " << N << ": " << p << " и " << q << "!\n";
    std::cout << "      Проверка: " << p << " * " << q << " = " << (p * q) << "\n\n";
}

int main() {
    std::cout << "====================================================================\n";
    std::cout << " Лабораторная работа №6 (КМЗИ) - Вариант 7\n";
    std::cout << " Алгоритмы Дойча, Дойча-Йожи. Алгоритм Шора\n";
    std::cout << "====================================================================\n\n";

    testDeutsch();
    testDeutschJozsa();
    testShorVariant7();

    std::cout << "====================================================================\n";
    std::cout << "[+] Все алгоритмы и тесты Лабораторной работы №6 успешно выполнены!\n";
    std::cout << "====================================================================\n";
    return 0;
}
