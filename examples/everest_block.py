import xobjects as xo
import xpart as xp
import xcoll as xc

import matplotlib.pyplot as plt

block = xc.EverestBlock(length=1., material=xc.materials.Tungsten)

part = xp.Particles(x=np.zeros(1000000), energy0=450.e9)
part._init_random_number_generator()
block.track(part)

_ = plt.hist(part.x, bins=200, density=True)
_ = plt.hist(part.px*part.rpp, bins=200, density=True)
