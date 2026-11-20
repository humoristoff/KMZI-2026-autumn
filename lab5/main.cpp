#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <cassert>
#include <random>
#include <algorithm>

// ============================================================================
// Лабораторная работа №5: Квантовые сумматоры. Алгоритм Гровера
// Вариант 7
// - Функция: F(A, B, C, D) = (A AND B) OR (C AND D) OR (A AND C)
// - База Гровера: N = 2^5 = 32, отмеченное состояние |10010> (18 в десятичной)
// ============================================================================

// --- Задание 1: Квантовые сумматоры ---
struct FullAdderCircuit {
    // 5 кубитов: q0=A, q1=B, q2=X (carry in), q3=S (sum), q4=C (carry out)
    static void run(int A, int B, int X, int& S, int& C) {
        int q0 = A, q1 = B, q2 = X;
        int q3 = 0, q4 = 0;

        // Вычисление суммы: S = A ^ B ^ X
        // CX(q0, q3), CX(q1, q3), CX(q2, q3)
        q3 ^= q0;
        q3 ^= q1;
        q3 ^= q2;

        // Вычисление переноса: C = (A & B) | (X & (A ^ B))
        // 1. CCX(q0, q1, q4) -> q4 = A & B
        if (q0 && q1) q4 ^= 1;
        // 2. CX(q0, q1) -> q1 = A ^ B
        q1 ^= q0;
        // 3. CCX(q1, q2, q4) -> q4 = (A & B) ^ (X & (A ^ B))
        if (q1 && q2) q4 ^= 1;
        // 4. CX(q0, q1) -> восстановление q1 = B
        q1 ^= q0;

        S = q3;
        C = q4;
    }
};

void testFullAdder() {
    std::cout << "[Задание 1] Проверка полного 5-кубитного сумматора:\n";
    std::cout << " A | B | X | Ожид (S, C) | Получ (S, C) | Статус\n";
    std::cout << "---+---+---+-------------+--------------+--------\n";
    for (int a = 0; a <= 1; ++a) {
        for (int b = 0; b <= 1; ++b) {
            for (int x = 0; x <= 1; ++x) {
                int expS = (a + b + x) % 2;
                int expC = (a + b + x) / 2;
                int actS = 0, actC = 0;
                FullAdderCircuit::run(a, b, x, actS, actC);
                assert(actS == expS && actC == expC);
                std::cout << " " << a << " | " << b << " | " << x
                          << " |      (" << expS << ", " << expC << ")  |       ("
                          << actS << ", " << actC << ")   | OK\n";
            }
        }
    }
    std::cout << "[+] Полный сумматор успешно протестирован на всех 8 комбинациях!\n\n";
}

void test4BitAdder() {
    std::cout << "[Задание 1.4] Тестирование 4-битного каскадного сумматора (256 пар чисел):\n";
    for (int a = 0; a < 16; ++a) {
        for (int b = 0; b < 16; ++b) {
            int carry = 0;
            int sum = 0;
            for (int bit = 0; bit < 4; ++bit) {
                int bitA = (a >> bit) & 1;
                int bitB = (b >> bit) & 1;
                int sBit = 0, nextCarry = 0;
                FullAdderCircuit::run(bitA, bitB, carry, sBit, nextCarry);
                sum |= (sBit << bit);
                carry = nextCarry;
            }
            sum |= (carry << 4);
            assert(sum == a + b);
        }
    }
    std::cout << "[+] Все 256 пар 4-битных сложений проверены программно (assert(sum == a + b))!\n\n";
}

// --- Задание 2: Квантовая схема варианта 7 ---
// F(A, B, C, D) = (A & B) | (C & D) | (A & C)
bool classicalF7(int A, int B, int C, int D) {
    return (A && B) || (C && D) || (A && C);
}

// Модульные обратимые квантовые вентили
void q_and(int in1, int in2, int& out) {
    if (in1 && in2) out ^= 1; // CCX
}

void q_or(int in1, int in2, int& out) {
    // a | b = !(!a & !b)
    // Реализация: X(in1), X(in2), CCX(in1, in2, out), X(out), X(in1), X(in2)
    int not1 = in1 ^ 1;
    int not2 = in2 ^ 1;
    int nandVal = 0;
    if (not1 && not2) nandVal ^= 1;
    out ^= (nandVal ^ 1);
}

// Квантовая схема с Uncomputation (развертыванием)
void quantumF7(int A, int B, int C, int D, int& out, std::vector<int>& ancilla) {
    // Вспомогательные кубиты:
    // anc[0]: t1 = A & B
    // anc[1]: t2 = C & D
    // anc[2]: t3 = A & C
    // anc[3]: t12 = t1 | t2
    // anc[4]: F_val = t12 | t3
    ancilla.assign(5, 0);

    // Прямое вычисление
    q_and(A, B, ancilla[0]);
    q_and(C, D, ancilla[1]);
    q_and(A, C, ancilla[2]);
    q_or(ancilla[0], ancilla[1], ancilla[3]);
    q_or(ancilla[3], ancilla[2], ancilla[4]);

    // Копирование на выходной кубит через CX
    out = ancilla[4];

    // Uncomputation (обратное вычисление для возврата всех ancilla в |0>)
    q_or(ancilla[3], ancilla[2], ancilla[4]);
    q_or(ancilla[0], ancilla[1], ancilla[3]);
    q_and(A, C, ancilla[2]);
    q_and(C, D, ancilla[1]);
    q_and(A, B, ancilla[0]);
}

void testVariant7Function() {
    std::cout << "[Задание 2] Тестирование функции F(A, B, C, D) = (A & B) | (C & D) | (A & C):\n";
    std::cout << " A B C D | F_классич | F_квантов | Вспом. кубиты в |0> | Статус\n";
    std::cout << "---------+-----------+-----------+---------------------+--------\n";
    for (int A = 0; A <= 1; ++A) {
        for (int B = 0; B <= 1; ++B) {
            for (int C = 0; C <= 1; ++C) {
                for (int D = 0; D <= 1; ++D) {
                    bool expF = classicalF7(A, B, C, D);
                    int actF = 0;
                    std::vector<int> anc;
                    quantumF7(A, B, C, D, actF, anc);

                    bool allZero = true;
                    for (int x : anc) if (x != 0) allZero = false;

                    assert(actF == (int)expF);
                    assert(allZero);

                    std::cout << " " << A << " " << B << " " << C << " " << D
                              << " |     " << expF << "     |     " << actF
                              << "     |         " << (allZero ? "Да (|00000>)" : "Нет")
                              << "         | OK\n";
                }
            }
        }
    }
    std::cout << "[+] Все 16 наборов проверены! Uncomputation вернул все вспомогательные кубиты в |0>.\n\n";
}

// --- Задания 4-7: Алгоритм Гровера (Вариант 7) ---
void runGroverVariant7() {
    const int n = 5;
    const int N = 1 << n; // 32
    const int marked = 0b10010; // 18 ("10010")

    std::cout << "[Задание 7] Моделирование алгоритма Гровера (Вариант 7):\n";
    std::cout << "  Число кубитов n = " << n << ", Размер базы N = " << N << "\n";
    std::cout << "  Отмеченное состояние |w> = |10010> (18)\n";

    double k_theor = std::floor((M_PI / 4.0) * std::sqrt(N));
    std::cout << "  Теоретическое оптимальное число итераций k_теор = floor(pi/4 * sqrt(32)) = " << (int)k_theor << "\n\n";

    std::vector<double> state(N, 1.0 / std::sqrt(N));

    std::cout << "  k | Точная вероятность P_точн(k) | Симуляция (1024 shots) | Примечание\n";
    std::cout << "----+------------------------------+------------------------+------------\n";

    int maxK = 0;
    double maxProb = 0.0;

    std::mt19937 rng(42);

    for (int k = 0; k <= 15; ++k) {
        double prob = state[marked] * state[marked];
        if (prob > maxProb) {
            maxProb = prob;
            maxK = k;
        }

        // Выборка 1024 измерений
        int countTarget = 0;
        std::discrete_distribution<int> dist(state.begin(), state.end());
        for (int s = 0; s < 1024; ++s) {
            int measured = dist(rng);
            if (measured == marked) countTarget++;
        }
        double expProb = (double)countTarget / 1024.0;

        std::cout << " " << std::setw(2) << k << " |           "
                  << std::fixed << std::setprecision(5) << prob << "            |         "
                  << std::setprecision(5) << expProb << "        | "
                  << (k == (int)k_theor ? "<-- ТЕОРЕТИЧЕСКИЙ ОПТИМУМ" : "") << "\n";

        // Grover iteration
        // 1. Phase oracle: инверсия фазы отмеченного состояния
        state[marked] = -state[marked];

        // 2. Diffusion operator: 2|s><s| - I
        double mean = 0.0;
        for (double a : state) mean += a;
        mean /= N;
        for (int i = 0; i < N; ++i) {
            state[i] = 2.0 * mean - state[i];
        }
    }

    std::cout << "\n[+] Итог задания 7:\n";
    std::cout << "    - k_теор:          " << (int)k_theor << "\n";
    std::cout << "    - k_точн (максимум): " << maxK << "\n";
    std::cout << "    - Максимальная P:  " << std::fixed << std::setprecision(5) << maxProb * 100.0 << "%\n";
    assert(maxK == (int)k_theor);
    std::cout << "    - Подтверждено: k_точн строго совпадает с k_теор!\n\n";
}

int main() {
    std::cout << "====================================================================\n";
    std::cout << " Лабораторная работа №5 (КМЗИ) - Вариант 7\n";
    std::cout << " Квантовые сумматоры. Алгоритм Гровера\n";
    std::cout << "====================================================================\n\n";

    testFullAdder();
    test4BitAdder();
    testVariant7Function();
    runGroverVariant7();

    std::cout << "====================================================================\n";
    std::cout << "[+] Все модули и тесты Лабораторной работы №5 успешно пройдены!\n";
    std::cout << "====================================================================\n";
    return 0;
}
