import unittest
import libnumbuf
import numpy as np
from numpy.testing import assert_equal

class SerializationTests(unittest.TestCase):

  def roundTripTest(self, data):
    serialized = libnumbuf.serialize_list(data)
    result = libnumbuf.deserialize_list(serialized)
    assert_equal(data, result)

  def testSimple(self):
    self.roundTripTest([1, 2, 3])
    self.roundTripTest([1.0, 2.0, 3.0])
    self.roundTripTest(['hello', 'world'])
    self.roundTripTest([1, 'hello', 1.0])
    self.roundTripTest([{'hello': 1.0, 'world': 42}])
    self.roundTripTest([True, False])

  def testNone(self):
    self.roundTripTest([1, 2, None, 3])

  def testNested(self):
    self.roundTripTest([{"hello": {"world": (1, 2, 3)}}])
    self.roundTripTest([((1,), (1, 2, 3, (4, 5, 6), "string"))])
    self.roundTripTest([{"hello": [1, 2, 3]}])
    self.roundTripTest([{"hello": [1, [2, 3]]}])
    self.roundTripTest([{"hello": (None, 2, [3, 4])}])
    self.roundTripTest([{"hello": (None, 2, [3, 4], np.ndarray([1.0, 2.0, 3.0]))}])

if __name__ == "__main__":
    unittest.main()
