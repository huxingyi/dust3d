"""Regression tests for the interpolation used by animation quality checks."""
import math
import sys
from pathlib import Path
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from dust3d_agent import glb


class RotationSamplingTests(unittest.TestCase):
    def test_quarter_interval_has_constant_angular_speed(self):
        # A 120-degree rotation must reach 30 degrees at t=.25. Normalized
        # linear interpolation only reaches 27.8 degrees, hiding export defects.
        channel = {'path': 'rotation', 'times': np.array([0., 1.]),
                   'values': np.array([[0., 0., 0., 1.], [0., math.sin(math.pi/3), 0., .5]])}
        expected = np.array([0., math.sin(math.pi/12), 0., math.cos(math.pi/12)])
        np.testing.assert_allclose(glb._sample(channel, .25), expected, atol=1e-8)
        channel['values'][1] *= -1
        np.testing.assert_allclose(glb._sample(channel, .25), expected, atol=1e-8)


if __name__ == '__main__':
    unittest.main()
