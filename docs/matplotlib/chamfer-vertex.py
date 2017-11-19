import matplotlib.pyplot as plt
import numpy as np
import math

def lerp(a, b, frac):
  return a + (b - a) * frac

def normal(x, y):
    val = math.sqrt(x*x + y*y)
    return (x/val, y/val)

def drawHalfArrow(fromX, fromY, toX, toY, color='lightgrey', alpha=0.8, offset=0.2):
    nor = normal(toX - fromX, toY - fromY)
    # shrink
    fromX += nor[0] * offset
    toX -= nor[0] * (0.2 + offset)
    fromY += nor[1] * offset
    toY -= nor[1] * (0.2 + offset)
    # move offset
    fromX += -nor[1] * offset
    fromY += nor[0] * offset
    toX += -nor[1] * offset
    toY += nor[0] * offset
    plt.arrow(fromX, fromY, toX - fromX, toY - fromY,
              fc=color, ec='k', alpha=alpha, width=0.1, head_width=2.5*0.1,
              head_length=2*0.1, head_starts_at_zero=True, length_includes_head=True,
              shape='right')

removingX = np.array([5])
removingY = np.array([5])

endpointX = np.array([1, 8,   7.2, 1.9])
endpointY = np.array([1, 1.5, 8.5, 8.4])

frac = 0.3
newX = np.array([])
newY = np.array([])
for i in range(0, 4):
    x = lerp(removingX.item(0), endpointX.item(i), frac)
    y = lerp(removingY.item(0), endpointY.item(i), frac)
    newX = np.append(newX, x)
    newY = np.append(newY, y)

plt.plot(removingX, removingY, marker='o', markersize=12, linewidth=0, alpha=0.7, color='k')
plt.plot(newX, newY, marker='o', markersize=12, linewidth=0, alpha=0.5, color='k')
plt.fill(newX, newY, linewidth=0, alpha=0.2, color='k')

for i in range(0, 4):
    plt.plot([endpointX.item(i), removingX.item(0)], [endpointY.item(i), removingY.item(0)], color='k', linestyle='-', alpha=0.3, linewidth=1)
    drawHalfArrow(newX.item(i), newY.item(i), endpointX.item(i), endpointY.item(i), alpha=0.3)
    drawHalfArrow(endpointX.item((i + 1) % 4), endpointY.item((i + 1) % 4), newX.item((i + 1) % 4), newY.item((i + 1) % 4), alpha=0.3)
    drawHalfArrow(newX.item((i + 1) % 4), newY.item((i + 1) % 4), newX.item(i), newY.item(i), alpha=0.6)
    drawHalfArrow(newX.item(i), newY.item(i), newX.item((i + 1) % 4), newY.item((i + 1) % 4), alpha=0.6)

ax = plt.gca()

ax.spines['right'].set_color('none')
ax.spines['bottom'].set_color('none')
ax.spines['left'].set_color('none')
ax.spines['top'].set_color('none')

plt.xlim(0, 10)
plt.ylim(0, 10)
plt.tick_params(labelbottom='off', labelleft='off', left='off', right='off', bottom='off', top='off')

plt.show()
