import unittest
import libnumbuf

class SerializationTests(unittest.TestCase):

  def roundTripTest(self, data):
    serialized = libnumbuf.serialize_list(data)
    result = libnumbuf.deserialize_list(serialized)
    self.assertEqual(data, result)

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
    self.roundTripTest([{"hello": {"world": 1}}])
    self.roundTripTest([{"hello": [1, 2, 3]}])
    self.roundTripTest([{"hello": [1, [2, 3]]}])

if __name__ == "__main__":
    unittest.main()
