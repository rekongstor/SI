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


def outInit(i):
    x = i // (GI_RESOLUTION ** 2)
    y = (i // GI_RESOLUTION) % GI_RESOLUTION
    z = i % GI_RESOLUTION
    return img[z, x * GI_RESOLUTION + y] / 255.0 * 2.0 - 1.0

inputs = torch.FloatTensor([[inpInit(i, j) for i in range(GI_RESOLUTION ** 3)] for j in range(3)])
outputs = img.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0

neuronsCount = 32
neuronsCountN2 = 32
# weights1 [32, 3]
# weights2 [32, 32]
# out weights [1, 32]

alpha = 0.9
beta1 = 0.9
beta2 = 0.001
acc = 0.9
rng = 0.1
steps = 10000
stepsN2 = 1000
pr = 100

data_x = Variable(inputs.cuda())
data_y = Variable(outputs.cuda())

torch.manual_seed(0)

w1 = Variable(torch.FloatTensor(neuronsCount, 3).cuda().uniform_(-rng, rng), requires_grad=True)
b1 = Variable(torch.FloatTensor(neuronsCount, 1).cuda().uniform_(-rng, rng), requires_grad=True)
w2 = Variable(torch.FloatTensor(neuronsCount, neuronsCount).cuda().uniform_(-rng, rng), requires_grad=True)
b2 = Variable(torch.FloatTensor(neuronsCount, 1).cuda().uniform_(-rng, rng), requires_grad=True)
w3 = Variable(torch.FloatTensor(1, neuronsCount).cuda().uniform_(-rng, rng), requires_grad=True)
b3 = Variable(torch.FloatTensor(1, 1).cuda().uniform_(-rng, rng), requires_grad=True)


def fc(weights, bias, inp):
    h1 = weights @ inp + bias
    a1 = torch.tanh(h1)

    return a1

def fcL(weights, bias, inp):
    h1 = weights @ inp + bias

    return h1


def Grad(s):
    s.data.add_(s.grad.data.mul(-alpha))
    s.grad.data.zero_()


def Forward(data_inp):
    n1 = fc(w1, b1, data_inp)
    n2 = fc(w2, b2, n1)
    y = fc(w3, b3, n2)

    return y


def Train(p, dat_x, dat_y):
    y = Forward(dat_x)
    loss = torch.mean((y - dat_y) ** 2)

    F = loss
    F.backward()
    Grad(w1)
    Grad(b1)
    Grad(w2)
    Grad(b2)
    Grad(w3)
    Grad(b3)
    return loss


def lerp(x, y, alpha):
    l = y * alpha + x * (1 - alpha)
    return l


for i in range(steps):
    ppr = i % pr == 0 or i == steps - 1
    loss = Train(ppr, data_x, data_y)

    alpha = lerp(alpha, lerp(beta1, beta2, (i / steps) ** 2), 1 - acc)
    if ppr:
        print(loss)
        print(str(i) + " " + str(alpha))


# Train NN weights
# neuronsCount * 3
# neuronsCount * 1
# neuronsCount * neuronsCount
# neuronsCount * 1
# 1 * neuronsCount
# 1 * 1

inputsN2 = outputs.resize(GI_RESOLUTION ** 3, 1)  # there should be ray-traced distances
outputsN2 = w2.resize(neuronsCount * neuronsCount, 1)

data_x2 = Variable(inputsN2.cuda())
data_y2 = Variable(outputsN2.cuda())

w1N2 = Variable(torch.FloatTensor(neuronsCountN2, GI_RESOLUTION ** 3).cuda().uniform_(-rng, rng), requires_grad=True)
b1N2 = Variable(torch.FloatTensor(neuronsCountN2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
w2N2 = Variable(torch.FloatTensor(neuronsCountN2, neuronsCountN2).cuda().uniform_(-rng, rng), requires_grad=True)
b2N2 = Variable(torch.FloatTensor(neuronsCountN2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
w3N2 = Variable(torch.FloatTensor(neuronsCount * neuronsCount, neuronsCountN2).cuda().uniform_(-rng, rng), requires_grad=True)
b3N2 = Variable(torch.FloatTensor(neuronsCount * neuronsCount, 1).cuda().uniform_(-rng, rng), requires_grad=True)


def ForwardN2(data_inp):
    n1 = fc(w1N2, b1N2, data_inp)
    n2 = fc(w2N2, b2N2, n1)
    y = fcL(w3N2, b3N2, n2)

    return y

def TrainN2(p, dat_x, dat_y):
    y = ForwardN2(dat_x)
    loss = torch.mean((y - dat_y) ** 2)

    F = loss
    F.backward()
    Grad(w1N2)
    Grad(b1N2)
    Grad(w2N2)
    Grad(b2N2)
    Grad(w3N2)
    Grad(b3N2)
    return loss

# N2 training
for i in range(stepsN2):
    ppr = i % pr == 0 or i == steps - 1
    loss = TrainN2(ppr, data_x2, data_y2)

    alpha = lerp(alpha, lerp(beta1, beta2, (i / stepsN2) ** 2), 1 - acc)
    if ppr:
        print(loss)
        print(str(i) + " " + str(alpha))

t1 = torch.clamp(Forward(data_x), 0, 1)
w2 = ForwardN2(data_x2).resize(neuronsCount, neuronsCount)

t2 = torch.clamp(Forward(data_x), 0, 1)

test1 = t1.resize(GI_RESOLUTION, GI_RESOLUTION ** 2)
test2 = t2.resize(GI_RESOLUTION, GI_RESOLUTION ** 2)

cv2.imwrite("res.png", test1.detach().cpu().numpy() * 255)
cv2.imshow('Result', test1.detach().cpu().numpy())
cv2.imshow('Result2', test2.detach().cpu().numpy())
# cv2.imshow('Original', img.detach().cpu().numpy())
cv2.waitKey()
