import numpy as np
import matplotlib.pylab as plt

data = np.loadtxt('imu_data_collection.csv', delimiter=',')

# Plot all the data.
fig, axes = plt.subplots(4, 3, figsize=(15, 10), sharex=True)

t = np.linspace(0, (data.shape[0] - 1) * 0.001, data.shape[0])

row_labels = [
    'accelerometer',
    'gyroscope',
    'attitude',
    'linear_acceleration'
]

for row in range(len(row_labels)):
    for iaxes in range(3):
        ax = axes[row][iaxes]
        ax.grid()
        ax.set_title(row_labels[row] + ' [' + ['x', 'y', 'z'][iaxes] + ']')
        ax.plot(t, data[:, 3 * row + iaxes])
        
fig.tight_layout()
fig.savefig('example_imu_all_data.pdf')

# Plot the last 30 data points / zooming onto the data.
POINT_TO_PLOT = 30
POINT_START = len(data) - POINT_TO_PLOT

fig, axes = plt.subplots(4, 3, figsize=(15, 10), sharex=True)

row_labels = [
    'accelerometer',
    'gyroscope',
    'attitude',
    'linear_acceleration'
]

for row in range(len(row_labels)):
    for iaxes in range(3):
        ax = axes[row][iaxes]
        ax.grid()
        ax.set_title(row_labels[row] + ' [' + ['x', 'y', 'z'][iaxes] + ']')
        ax.plot(t[POINT_START:], data[:, 3 * row + iaxes][POINT_START:])
        
fig.tight_layout()
fig.savefig('example_imu_last_few_data.pdf')
