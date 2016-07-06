import libplasma
import pandas as pd

plasma = libplasma.connect("/home/pcmoritz/server", "72BBF1CF737684BE7FAD58A8E5780479")

A = libplasma.build_object(plasma, 1, 1000, "object-1")
B = libplasma.build_object(plasma, 2, 2000, "object-2")

names, sizes, timestamps = libplasma.list_object(plasma)

info = pd.DataFrame({"name": names, "size": sizes, "timestamp": timestamps})
