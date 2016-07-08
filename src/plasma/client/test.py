import libplasma
import os
# import pandas as pd

plasma = libplasma.connect("/home/pcmoritz/shell")

A = libplasma.build_object(plasma, 1, 1000, "object-1")
libplasma.seal_object(A)
B = libplasma.build_object(plasma, 2, 2000, "object-2")
libplasma.seal_object(B)

libplasma.list_objects(plasma)

names, sizes, create_times, construct_deltas, creator_ids = libplasma.list_objects(plasma)

info = pd.DataFrame({"name": names, "size": sizes, "create_time": create_times,
  "construct_deltas": construct_deltas, "creator_ids": creator_ids})
