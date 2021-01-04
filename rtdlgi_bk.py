import torch
import cv2
import math
import numpy
from torch.autograd import Variable

GI_RESOLUTION = 64
TRAINING_SAMPLES = 8

# training data
inputs = torch.zeros(GI_RESOLUTION**3, TRAINING_SAMPLES)
outputs = torch.zeros(GI_RESOLUTION**3, TRAINING_SAMPLES)

for i in range(TRAINING_SAMPLES):
    imgOrgDist = cv2.imread("rtOut\\Raytracing_output_Ray_Dist" + str(i) + ".png", cv2.IMREAD_COLOR)
    imgOrgGI = cv2.imread("rtOut\\Raytracing_output" + str(i) + ".png", cv2.IMREAD_COLOR)
    imgDist = torch.from_numpy(imgOrgDist[:, :, 2]).resize(GI_RESOLUTION**3) / 255.0 * 2.0 - 1.0
    imgGI = torch.from_numpy(imgOrgGI[:, :, 2]).resize(GI_RESOLUTION**3) / 255.0 * 2.0 - 1.0
    inputs[:, i] = imgDist
    outputs[:, i] = imgGI

def inpInit(i, j):
    xB = int(j % 3 == 0)
    yB = int(j % 3 == 1)
    zB = int(j % 3 == 2)
    x = i // (GI_RESOLUTION ** 2)
    y = (i // GI_RESOLUTION) % GI_RESOLUTION
    z = i % GI_RESOLUTION
    res = xB * x + yB * y + zB * z
    return res / GI_RESOLUTION * 2.0 - 1.0


inputsXYZ = torch.FloatTensor([[inpInit(i, j) for i in range(GI_RESOLUTION ** 3)] for j in range(3)])

neuronsCount_Dist = 48
neuronsCount_GI = 32

# Train NN weights
# neuronsCount * 3
# neuronsCount * 1
# neuronsCount * neuronsCount
# neuronsCount * 1
# 1 * neuronsCount
# 1 * 1
reducer = 8
neuronsCount_Dist_w1 = neuronsCount_Dist // reducer * 3
neuronsCount_Dist_b1 = neuronsCount_Dist // reducer * 1
neuronsCount_Dist_w2 = neuronsCount_Dist // reducer * neuronsCount_Dist
neuronsCount_Dist_b2 = neuronsCount_Dist // reducer * 1
neuronsCount_Dist_w3 = neuronsCount_Dist // reducer * 1
neuronsCount_Dist_b3 = reducer * 1

alpha = 0.01
beta1 = 0.01
beta2 = 0.000001
tanhMul = 1
acc = 0.1
rng = 0.1
steps = 10000
pr = 10

data_x_Dist = Variable(inputs.cuda())
data_x_XYZ = Variable(inputsXYZ.cuda())
data_y_GI = Variable(outputs.cuda())

torch.manual_seed(0)

WEIGHTS1_w1 = Variable(torch.FloatTensor(neuronsCount_Dist_w1, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_w1 = Variable(torch.FloatTensor(neuronsCount_Dist_w1, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_w1 = Variable(torch.FloatTensor(neuronsCount_Dist_w1, neuronsCount_Dist_w1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_w1 = Variable(torch.FloatTensor(neuronsCount_Dist_w1, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_w1 = Variable(torch.FloatTensor(neuronsCount_GI * 3, neuronsCount_Dist_w1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_w1 = Variable(torch.FloatTensor(neuronsCount_GI * 3, 1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS1_b1 = Variable(torch.FloatTensor(neuronsCount_Dist_b1, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_b1 = Variable(torch.FloatTensor(neuronsCount_Dist_b1, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_b1 = Variable(torch.FloatTensor(neuronsCount_Dist_b1, neuronsCount_Dist_b1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_b1 = Variable(torch.FloatTensor(neuronsCount_Dist_b1, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_b1 = Variable(torch.FloatTensor(neuronsCount_GI * 1, neuronsCount_Dist_b1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_b1 = Variable(torch.FloatTensor(neuronsCount_GI * 1, 1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS1_w2 = Variable(torch.FloatTensor(neuronsCount_Dist_w2, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_w2 = Variable(torch.FloatTensor(neuronsCount_Dist_w2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_w2 = Variable(torch.FloatTensor(neuronsCount_Dist_w2, neuronsCount_Dist_w2).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_w2 = Variable(torch.FloatTensor(neuronsCount_Dist_w2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_w2 = Variable(torch.FloatTensor(neuronsCount_GI * neuronsCount_GI, neuronsCount_Dist_w2).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_w2 = Variable(torch.FloatTensor(neuronsCount_GI * neuronsCount_GI, 1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS1_b2 = Variable(torch.FloatTensor(neuronsCount_Dist_b2, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_b2 = Variable(torch.FloatTensor(neuronsCount_Dist_b2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_b2 = Variable(torch.FloatTensor(neuronsCount_Dist_b2, neuronsCount_Dist_b2).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_b2 = Variable(torch.FloatTensor(neuronsCount_Dist_b2, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_b2 = Variable(torch.FloatTensor(neuronsCount_GI * 1, neuronsCount_Dist_b2).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_b2 = Variable(torch.FloatTensor(neuronsCount_GI * 1, 1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS1_w3 = Variable(torch.FloatTensor(neuronsCount_Dist_w3, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_w3 = Variable(torch.FloatTensor(neuronsCount_Dist_w3, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_w3 = Variable(torch.FloatTensor(neuronsCount_Dist_w3, neuronsCount_Dist_w3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_w3 = Variable(torch.FloatTensor(neuronsCount_Dist_w3, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_w3 = Variable(torch.FloatTensor(1 * neuronsCount_GI, neuronsCount_Dist_w3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_w3 = Variable(torch.FloatTensor(1 * neuronsCount_GI, 1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS1_b3 = Variable(torch.FloatTensor(neuronsCount_Dist_b3, GI_RESOLUTION**3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1_b3 = Variable(torch.FloatTensor(neuronsCount_Dist_b3, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS2_b3 = Variable(torch.FloatTensor(neuronsCount_Dist_b3, neuronsCount_Dist_b3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2_b3 = Variable(torch.FloatTensor(neuronsCount_Dist_b3, 1).cuda().uniform_(-rng, rng), requires_grad=True)
WEIGHTS3_b3 = Variable(torch.FloatTensor(1, neuronsCount_Dist_b3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3_b3 = Variable(torch.FloatTensor(1, 1).cuda().uniform_(-rng, rng), requires_grad=True)


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


def Forward(data_inp, w1, b1, w2, b2, w3, b3):
    n1 = fc(w1, b1, data_inp)
    n2 = fc(w2, b2, n1)
    y = fc(w3, b3, n2)

    return y


def Forward_FCL(data_inp, w1, b1, w2, b2, w3, b3):
    n1 = fc(w1, b1, data_inp)
    n2 = fc(w2, b2, n1)
    y = fcL(w3, b3, n2)

    return y

def Forward_N2(dat_x):
    w1_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_w1, BIAS1_w1, WEIGHTS2_w1, BIAS2_w1, WEIGHTS3_w1, BIAS3_w1).resize(neuronsCount_GI, 3)
    b1_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_b1, BIAS1_b1, WEIGHTS2_b1, BIAS2_b1, WEIGHTS3_b1, BIAS3_b1).resize(neuronsCount_GI, 1)
    w2_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_w2, BIAS1_w2, WEIGHTS2_w2, BIAS2_w2, WEIGHTS3_w2, BIAS3_w2).resize(neuronsCount_GI, neuronsCount_GI)
    b2_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_b2, BIAS1_b2, WEIGHTS2_b2, BIAS2_b2, WEIGHTS3_b2, BIAS3_b2).resize(neuronsCount_GI, 1)
    w3_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_w3, BIAS1_w3, WEIGHTS2_w3, BIAS2_w3, WEIGHTS3_w3, BIAS3_w3).resize(1, neuronsCount_GI)
    b3_1 = Forward_FCL(dat_x.resize(GI_RESOLUTION**3, 1), WEIGHTS1_b3, BIAS1_b3, WEIGHTS2_b3, BIAS2_b3, WEIGHTS3_b3, BIAS3_b3).resize(1, 1)

    y = Forward(data_x_XYZ, w1_1, b1_1, w2_1, b2_1, w3_1, b3_1)

    return y

def Train(p, ts):
    loss = 0
    for i in range(ts):
        Y = Forward_N2(data_x_Dist[:, i])
        loss += torch.mean((Y - data_y_GI[:, i]) ** 2)

    loss.backward()
    Grad(WEIGHTS1_w1)
    Grad(BIAS1_w1)
    Grad(WEIGHTS2_w1)
    Grad(BIAS2_w1)
    Grad(WEIGHTS3_w1)
    Grad(BIAS3_w1)

    Grad(WEIGHTS1_b1)
    Grad(BIAS1_b1)
    Grad(WEIGHTS2_b1)
    Grad(BIAS2_b1)
    Grad(WEIGHTS3_b1)
    Grad(BIAS3_b1)

    Grad(WEIGHTS1_w2)
    Grad(BIAS1_w2)
    Grad(WEIGHTS2_w2)
    Grad(BIAS2_w2)
    Grad(WEIGHTS3_w2)
    Grad(BIAS3_w2)

    Grad(WEIGHTS1_b2)
    Grad(BIAS1_b2)
    Grad(WEIGHTS2_b2)
    Grad(BIAS2_b2)
    Grad(WEIGHTS3_b2)
    Grad(BIAS3_b2)

    Grad(WEIGHTS1_w3)
    Grad(BIAS1_w3)
    Grad(WEIGHTS2_w3)
    Grad(BIAS2_w3)
    Grad(WEIGHTS3_w3)
    Grad(BIAS3_w3)

    Grad(WEIGHTS1_b3)
    Grad(BIAS1_b3)
    Grad(WEIGHTS2_b3)
    Grad(BIAS2_b3)
    Grad(WEIGHTS3_b3)
    Grad(BIAS3_b3)
    return loss


def lerp(x, y, alpha):
    l = y * alpha + x * (1 - alpha)
    return l


for i in range(steps):
    ppr = i % pr == 0 or i == steps - 1
    loss = Train(ppr, TRAINING_SAMPLES)

    alpha = lerp(alpha, lerp(beta1, beta2, math.tanh(i / steps * tanhMul)), 1 - acc)
    if ppr:
        print(loss / 8)
        print(str(i) + " " + str(alpha))


# Train NN weights
# neuronsCount * 3
# neuronsCount * 1
# neuronsCount * neuronsCount
# neuronsCount * 1
# 1 * neuronsCount
# 1 * 1

def DrawRes(res, name):
    t1 = res.resize(GI_RESOLUTION, GI_RESOLUTION ** 2)
    cv2.imwrite(name, t1.detach().cpu().numpy() * 255)
    cv2.imshow(name, t1.detach().cpu().numpy())

for i in range(TRAINING_SAMPLES):
    m = torch.clamp(Forward_N2(data_x_Dist[:,i]), 0, 1)
    DrawRes(m, "m"+str(i)+".png")

cv2.waitKey()

