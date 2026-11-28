The target function is:

$$$
f(x) = \frac{0.3x + 0.1}{x^2 + 0.5}
$$$

Dataset characteristics:

- number of samples: 200;
- sampling method: uniform random distribution using Python's `random.Random(42)`;
- input range: $x\in[-3,3]$;
- column order: target, x;
- noise: none;
- numerical precision: 17 significant digits (double-precision floating-point).

The denominator is at least 0.5, so the function has no real singularities.
The task tests rational expressions with small coefficients. The search success
threshold is a fitness of at least -0.01.
