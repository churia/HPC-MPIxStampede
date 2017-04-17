import matplotlib
import matplotlib.pylab as plt
import numpy as np
import sys

font = {'family' : 'normal',
        'weight' : 'bold',
        'size'   : 16}

matplotlib.rc('font', **font)
matplotlib.rc('text', usetex=True)
matplotlib.rcParams['text.latex.preamble']=[r"\usepackage{amsmath}"]
'''
#weak1

x=np.array([1,4,16,64])
y=np.array([1.1905,1.2347,1.367,1.497])

#weak2

x=np.array([1,4,16,64,256,1024,4096])
y=np.array([143.35,145.34,160.67,167.88,172.74,198.71,317.25])

#str1#

x=np.array([1,4,16,64])
y=np.array([1803.68,463.60,117.67,33.46])
'''
'''
x=np.array([1,4,16,64])
#yb = np.array([143.36,145.12,160.17,162.43])
#ynb = np.array([143.95,146.34,163.18,211.26])

yb=np.array([1803.68,463.60,117.67,33.46])
ynb=np.array([1818.46,467.25,117.84,32.36])
'''
'''
fig, ax = plt.subplots()
ax.set_xscale('log', basex=4)
'''
#b,=plt.plot(x, yb, 'b', linewidth=2.4)
#nb, =plt.plot(x, ynb, 'g', linewidth=2.4)
#plt.ylim([0,300])

x=np.array([10,100,1000,10000,100000,1000000,10000000])
y=np.array([0.036945,0.020823,0.026867,0.074615,0.177274,0.808131,9.656340])
fig, ax = plt.subplots()
ax.set_xscale('log', basex=10)
#plt.legend([b,nb],['blocking','non-blocking'])
plt.xlabel('\\#random numbers stored per process')
plt.ylabel('time (sec)')
plt.ylim([0,10])
plt.xlim([10,10000000])
plt.plot(x, y, 'b', linewidth=2.4)
#ax.set_title('Jacobi2D-mpi, $N_l$=100, \\#iter=20000')
#ax.set_title('Jacobi2D-mpi, $N_l$=500, \\#iter=100000')
#ax.set_title('Jacobi2D-mpi, N=1024, \\#iter=300000')
ax.set_title('Sample sort')
plt.savefig('ssort.eps',format='eps', dpi=1000)
plt.show()