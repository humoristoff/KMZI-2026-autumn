"""
Лабораторная работа №5: Квантовые сумматоры. Алгоритм Гровера.
Вариант 7:
- Задание 2: F(A, B, C, D) = (A AND B) OR (C AND D) OR (A AND C)
- Задание 7: N = 2^5 = 32, отмеченное состояние |10010>
"""

import math
import numpy as np

# =====================================================================
# ЗАДАНИЕ 1: КВАНТОВЫЕ СУММАТОРЫ
# =====================================================================

def full_adder_logic(a: int, b: int, x: int):
    """
    Полный сумматор на 5 кубитах (A, B, X, S, C).
    Использует обратимые вентили CX и CCX.
    """
    q0, q1, q2 = a, b, x
    q3, q4 = 0, 0

    # Вычисление суммы S = A ^ B ^ X
    q3 ^= q0
    q3 ^= q1
    q3 ^= q2

    # Вычисление переноса C = (A & B) | (X & (A ^ B))
    if q0 and q1:
        q4 ^= 1
    q1 ^= q0
    if q1 and q2:
        q4 ^= 1
    q1 ^= q0  # uncompute q1 обратно в b

    return q3, q4

def test_full_adder():
    print("=" * 60)
    print("ЗАДАНИЕ 1: Тестирование полного сумматора")
    print("=" * 60)
    print(" A | B | X | Ожид (S, C) | Получ (S, C) | Статус")
    print("---+---+---+-------------+--------------+--------")
    for a in (0, 1):
        for b in (0, 1):
            for x in (0, 1):
                exp_s = (a + b + x) % 2
                exp_c = (a + b + x) // 2
                s, c = full_adder_logic(a, b, x)
                assert s == exp_s and c == exp_c, f"Ошибка для ({a},{b},{x})"
                print(f" {a} | {b} | {x} |      ({exp_s}, {exp_c})  |       ({s}, {c})   | OK")
    print("[+] Полный сумматор успешно протестирован на всех 8 комбинациях!\n")

def test_4bit_adder():
    print("[1.4] Тестирование 4-разрядного сумматора (256 комбинаций a+b):")
    for a in range(16):
        for b in range(16):
            carry = 0
            res = 0
            for bit in range(4):
                bit_a = (a >> bit) & 1
                bit_b = (b >> bit) & 1
                s_bit, carry = full_adder_logic(bit_a, bit_b, carry)
                res |= (s_bit << bit)
            res |= (carry << 4)
            assert res == a + b, f"Ошибка сложения: {a} + {b} != {res}"
    print("[+] Все 256 пар сложения от 0 до 15 проверены успешно (assert(res == a + b))!\n")

# =====================================================================
# ЗАДАНИЕ 2: КВАНТОВАЯ СХЕМА ВАРИАНТА 7
# F(A, B, C, D) = (A & B) | (C & D) | (A & C)
# =====================================================================

def classical_f7(a: int, b: int, c: int, d: int) -> int:
    return int((a and b) or (c and d) or (a and c))

def quantum_f7_simulation(a: int, b: int, c: int, d: int):
    """
    Модульная реализация квантовой схемы с uncomputation.
    Ancilla:
      anc0: A AND B
      anc1: C AND D
      anc2: A AND C
      anc3: anc0 OR anc1
      anc4: anc3 OR anc2 -> значение F
    Выход: out копирует anc4 через CX.
    Затем обратная схема (uncomputation) возвращает anc0..anc4 в |0>.
    """
    # Вспомогательные кубиты
    anc0 = a & b
    anc1 = c & d
    anc2 = a & c
    anc3 = anc0 | anc1
    anc4 = anc3 | anc2

    # Копирование на выходной кубит
    out = anc4

    # Uncomputation
    anc4_un = anc4 ^ (anc3 | anc2)
    anc3_un = anc3 ^ (anc0 | anc1)
    anc2_un = anc2 ^ (a & c)
    anc1_un = anc1 ^ (c & d)
    anc0_un = anc0 ^ (a & b)

    anc_state = [anc0_un, anc1_un, anc2_un, anc3_un, anc4_un]
    return out, anc_state

def test_variant7_function():
    print("=" * 60)
    print("ЗАДАНИЕ 2: Тестирование булевой схемы Варианта 7")
    print("F(A, B, C, D) = (A & B) | (C & D) | (A & C)")
    print("=" * 60)
    print(" A B C D | F_классич | F_квантов | Вспом. кубиты в |0> | Статус")
    print("---------+-----------+-----------+---------------------+--------")
    for a in (0, 1):
        for b in (0, 1):
            for c in (0, 1):
                for d in (0, 1):
                    exp_f = classical_f7(a, b, c, d)
                    act_f, anc_status = quantum_f7_simulation(a, b, c, d)
                    assert act_f == exp_f, f"Несовпадение F на наборе {a}{b}{c}{d}"
                    assert all(x == 0 for x in anc_status), "Вспомогательные кубиты не вернулись в |0>"
                    print(f" {a} {b} {c} {d} |     {exp_f}     |     {act_f}     |         Да (|00000>)         | OK")
    print("[+] Все 16 наборов проверены! Uncomputation вернул все вспомогательные кубиты в |0>.\n")

# =====================================================================
# ЗАДАНИЕ 4-7: АЛГОРИТМ ГРОВЕРА (ВАРИАНТ 7)
# N = 2^5 = 32, отмеченное состояние |10010> (18)
# =====================================================================

def build_oracle_matrix(n: int, marked_str: str) -> np.ndarray:
    """Диагональный фазовый оракул для отмеченного состояния."""
    N = 1 << n
    idx = int(marked_str, 2)
    diag = np.ones(N, dtype=np.float64)
    diag[idx] = -1.0
    return diag

def grover_simulation(n: int, marked_str: str, max_k: int = 15):
    N = 1 << n
    idx = int(marked_str, 2)
    oracle_diag = build_oracle_matrix(n, marked_str)

    # Равномерная суперпозиция |s>
    state = np.full(N, 1.0 / math.sqrt(N), dtype=np.float64)

    k_theor = int(math.floor((math.pi / 4.0) * math.sqrt(N)))

    print("=" * 60)
    print(f"ЗАДАНИЕ 7: Алгоритм Гровера (Вариант 7: N = 2^{n} = {N}, |w> = |{marked_str}>)")
    print(f"Теоретический оптимум k_теор = floor(pi/4 * sqrt({N})) = {k_theor}")
    print("=" * 60)
    print("  k | Точная вероятность P_точн(k) | Симуляция (1024 shots) | Примечание")
    print("----+------------------------------+------------------------+------------")

    probs = []
    sim_probs = []
    rng = np.random.default_rng(42)

    for k in range(max_k + 1):
        prob = state[idx] ** 2
        probs.append(prob)

        # Симуляция 1024 измерений
        counts = rng.multinomial(1024, state ** 2)
        sim_prob = counts[idx] / 1024.0
        sim_probs.append(sim_prob)

        marker = "<-- ТЕОРЕТИЧЕСКИЙ ОПТИМУМ" if k == k_theor else ""
        print(f" {k:2d} |           {prob:.5f}            |         {sim_prob:.5f}        | {marker}")

        # Grover iteration:
        # 1. Oracle: flip phase of marked state
        state *= oracle_diag
        # 2. Diffusion: 2|s><s| - I
        mean = np.mean(state)
        state = 2.0 * mean - state

    k_exact_max = int(np.argmax(probs))
    print("\n[+] Результаты:")
    print(f"    - k_теор:          {k_theor}")
    print(f"    - k_точн (максимум): {k_exact_max}")
    print(f"    - P_max(точн):     {probs[k_exact_max] * 100:.2f}%")
    assert k_exact_max == k_theor, "Оптимум не совпал с теорией!"
    print("    - Подтверждено: k_точн строго совпадает с k_теор!\n")

def main():
    test_full_adder()
    test_4bit_adder()
    test_variant7_function()
    grover_simulation(5, "10010", 15)
    print("=" * 60)
    print("[+] Все задания ЛР №5 успешно выполнены и проверены!")
    print("=" * 60)

if __name__ == "__main__":
    main()
