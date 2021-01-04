import torch
import cv2
import math
from torch.autograd import Variable

GI_RESOLUTION = 64

# training data
# RenderTargetOut[uint2(dr.x * GI_RESOLUTION + dr.y, dr.z)] = payload.color.x;
imgOrg = cv2.imread("giOut.png", cv2.IMREAD_COLOR)
img = torch.from_numpy(imgOrg[:, :, 2])


# inputs [batchsize, 3]
# outputs [batchsize, 1]
# inputs = torch.zeros(3, GI_RESOLUTION ** 3)  # [-1;+1]
# outputs = torch.zeros(1, GI_RESOLUTION ** 3)  # [-1;+1]

def inpInit(i, j):
    xB = int(j % 3 == 0)
    yB = int(j % 3 == 1)
    zB = int(j % 3 == 2)
    x = i // (GI_RESOLUTION ** 2)
    y = (i // GI_RESOLUTION) % GI_RESOLUTION
    z = i % GI_RESOLUTION
    res = xB * x + yB * y + zB * z
    return res / GI_RESOLUTION * 2.0 - 1.0


inputs = torch.FloatTensor([[inpInit(i, j) for i in range(GI_RESOLUTION ** 3)] for j in range(3)])
outputs = img.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0

l1_count = 32  # weights1 [l1_count, 3]
l2_count = 32  # weights2 [l2_count, l1_count]
# out weights [1, l2_count]

alpha = 0.9
beta1 = 0.9
beta2 = 0.001
acc = 0.9
rng = 0.01
steps = 40000
pr = 100

data_x = Variable(inputs.cuda())
data_y = Variable(outputs.cuda())

w1 = Variable(torch.FloatTensor(l1_count, 3).uniform_(-rng, rng).cuda(), requires_grad=True)
b1 = Variable(torch.FloatTensor(l1_count, 1).uniform_(-rng, rng).cuda(), requires_grad=True)
w2 = Variable(torch.FloatTensor(l2_count, l1_count).uniform_(-rng, rng).cuda(), requires_grad=True)
b2 = Variable(torch.FloatTensor(l2_count, 1).uniform_(-rng, rng).cuda(), requires_grad=True)
w3 = Variable(torch.FloatTensor(1, l2_count).uniform_(-rng, rng).cuda(), requires_grad=True)
b3 = Variable(torch.FloatTensor(1, 1).uniform_(-rng, rng).cuda(), requires_grad=True)

model = torch.nn.Sequential(
    torch.nn.Linear(data_x, l1_count),
    torch.nn.ReLU(),
    torch.nn.Linear(l1_count, l2_count),
    torch.nn.ReLU(),
    torch.nn.Linear()
)


def lerp(x, y, alpha):
    l = y * alpha + x * (1 - alpha)
    return l


for i in range(steps):
    ppr = i % pr == 0 or i == steps - 1
    loss = Train(ppr)

    alpha = lerp(alpha, lerp(beta1, beta2, (i / steps) ** 2), 1 - acc)
    if ppr:
        print(loss)
        print(str(i) + " " + str(alpha))

test = torch.zeros(GI_RESOLUTION, GI_RESOLUTION ** 2)

t1 = torch.clamp(Forward(data_x), 0, 1)

test = t1.resize(GI_RESOLUTION, GI_RESOLUTION ** 2)

cv2.imwrite("res.png", test.detach().cpu().numpy() * 255)
cv2.imshow('Result', test.detach().cpu().numpy())
# cv2.imshow('Original', img.detach().cpu().numpy())
cv2.waitKey()
