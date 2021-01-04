import torch
import cv2
import math
import numpy
from torch.autograd import Variable

GI_RESOLUTION = 64

# training data
# RenderTargetOut[uint2(dr.x * GI_RESOLUTION + dr.y, dr.z)] = payload.color.x;
imgOrgGI = cv2.imread("rtGI.png", cv2.IMREAD_COLOR)
imgOrgDist = cv2.imread("rtDist.png", cv2.IMREAD_COLOR)

imgGI0 = torch.from_numpy(imgOrgGI[0:64, :, 2])
imgGI1 = torch.from_numpy(imgOrgGI[64:128, :, 2])
imgGI2 = torch.from_numpy(imgOrgGI[128:192, :, 2])
imgGI3 = torch.from_numpy(imgOrgGI[192:256, :, 2])
imgGI4 = torch.from_numpy(imgOrgGI[256:320, :, 2])
imgGI5 = torch.from_numpy(imgOrgGI[320:384, :, 2])
imgGI6 = torch.from_numpy(imgOrgGI[384:448, :, 2])
imgGI7 = torch.from_numpy(imgOrgGI[448:512, :, 2])

imgDist0 = torch.from_numpy(imgOrgDist[0:64, :, 2])
imgDist1 = torch.from_numpy(imgOrgDist[64:128, :, 2])
imgDist2 = torch.from_numpy(imgOrgDist[128:192, :, 2])
imgDist3 = torch.from_numpy(imgOrgDist[192:256, :, 2])
imgDist4 = torch.from_numpy(imgOrgDist[256:320, :, 2])
imgDist5 = torch.from_numpy(imgOrgDist[320:384, :, 2])
imgDist6 = torch.from_numpy(imgOrgDist[384:448, :, 2])
imgDist7 = torch.from_numpy(imgOrgDist[448:512, :, 2])


def inpInit(i, j):
    xB = int(j % 3 == 0)
    yB = int(j % 3 == 1)
    zB = int(j % 3 == 2)
    x = i // (GI_RESOLUTION ** 2)
    y = (i // GI_RESOLUTION) % GI_RESOLUTION
    z = i % GI_RESOLUTION
    res = xB * x + yB * y + zB * z
    return res / GI_RESOLUTION * 2.0 - 1.0


inputsDist0 = imgDist0.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist1 = imgDist1.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist2 = imgDist2.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist3 = imgDist3.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist4 = imgDist4.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist5 = imgDist5.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist6 = imgDist6.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0
inputsDist7 = imgDist7.resize(GI_RESOLUTION ** 3, 1) / 255.0 * 2.0 - 1.0

inputsGI = torch.FloatTensor([[inpInit(i, j) for i in range(GI_RESOLUTION ** 3)] for j in range(3)])

outputsGI0 = imgGI0.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI1 = imgGI1.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI2 = imgGI2.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI3 = imgGI3.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI4 = imgGI4.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI5 = imgGI5.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI6 = imgGI6.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0
outputsGI7 = imgGI7.resize(1, GI_RESOLUTION ** 3) / 255.0 * 2.0 - 1.0

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

data_x_Dist0 = Variable(inputsDist0.cuda())
data_x_Dist1 = Variable(inputsDist1.cuda())
data_x_Dist2 = Variable(inputsDist2.cuda())
data_x_Dist3 = Variable(inputsDist3.cuda())
data_x_Dist4 = Variable(inputsDist4.cuda())
data_x_Dist5 = Variable(inputsDist5.cuda())
data_x_Dist6 = Variable(inputsDist6.cuda())
data_x_Dist7 = Variable(inputsDist7.cuda())

data_x_GI = Variable(inputsGI.cuda())

data_y_GI0 = Variable(outputsGI0.cuda())
data_y_GI1 = Variable(outputsGI1.cuda())
data_y_GI2 = Variable(outputsGI2.cuda())
data_y_GI3 = Variable(outputsGI3.cuda())
data_y_GI4 = Variable(outputsGI4.cuda())
data_y_GI5 = Variable(outputsGI5.cuda())
data_y_GI6 = Variable(outputsGI6.cuda())
data_y_GI7 = Variable(outputsGI7.cuda())

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
    w1_1 = Forward_FCL(dat_x, WEIGHTS1_w1, BIAS1_w1, WEIGHTS2_w1, BIAS2_w1, WEIGHTS3_w1, BIAS3_w1).resize(neuronsCount_GI, 3)
    b1_1 = Forward_FCL(dat_x, WEIGHTS1_b1, BIAS1_b1, WEIGHTS2_b1, BIAS2_b1, WEIGHTS3_b1, BIAS3_b1).resize(neuronsCount_GI, 1)
    w2_1 = Forward_FCL(dat_x, WEIGHTS1_w2, BIAS1_w2, WEIGHTS2_w2, BIAS2_w2, WEIGHTS3_w2, BIAS3_w2).resize(neuronsCount_GI, neuronsCount_GI)
    b2_1 = Forward_FCL(dat_x, WEIGHTS1_b2, BIAS1_b2, WEIGHTS2_b2, BIAS2_b2, WEIGHTS3_b2, BIAS3_b2).resize(neuronsCount_GI, 1)
    w3_1 = Forward_FCL(dat_x, WEIGHTS1_w3, BIAS1_w3, WEIGHTS2_w3, BIAS2_w3, WEIGHTS3_w3, BIAS3_w3).resize(1, neuronsCount_GI)
    b3_1 = Forward_FCL(dat_x, WEIGHTS1_b3, BIAS1_b3, WEIGHTS2_b3, BIAS2_b3, WEIGHTS3_b3, BIAS3_b3).resize(1, 1)

    y = Forward(data_x_GI, w1_1, b1_1, w2_1, b2_1, w3_1, b3_1)

    return y

def Train(p):
    m0 = Forward_N2(data_x_Dist0)
    m1 = Forward_N2(data_x_Dist1)
    m2 = Forward_N2(data_x_Dist2)
    m3 = Forward_N2(data_x_Dist3)
    m4 = Forward_N2(data_x_Dist4)
    m5 = Forward_N2(data_x_Dist5)
    m6 = Forward_N2(data_x_Dist6)
    m7 = Forward_N2(data_x_Dist7)

    loss = torch.mean((m0 - data_y_GI0) ** 2) + \
           torch.mean((m1 - data_y_GI1) ** 2) + \
           torch.mean((m2 - data_y_GI2) ** 2) + \
           torch.mean((m3 - data_y_GI3) ** 2) + \
           torch.mean((m4 - data_y_GI4) ** 2) + \
           torch.mean((m5 - data_y_GI5) ** 2) + \
           torch.mean((m6 - data_y_GI6) ** 2) + \
           torch.mean((m7 - data_y_GI7) ** 2)

    F = loss
    F.backward()
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
    loss = Train(ppr)

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

m0 = torch.clamp(Forward_N2(data_x_Dist0), 0, 1)
m1 = torch.clamp(Forward_N2(data_x_Dist1), 0, 1)
m2 = torch.clamp(Forward_N2(data_x_Dist2), 0, 1)
m3 = torch.clamp(Forward_N2(data_x_Dist3), 0, 1)
m4 = torch.clamp(Forward_N2(data_x_Dist4), 0, 1)
m5 = torch.clamp(Forward_N2(data_x_Dist5), 0, 1)
m6 = torch.clamp(Forward_N2(data_x_Dist6), 0, 1)
m7 = torch.clamp(Forward_N2(data_x_Dist7), 0, 1)

DrawRes(m0, "m0.png")
DrawRes(m1, "m1.png")
DrawRes(m2, "m2.png")
DrawRes(m3, "m3.png")
DrawRes(m4, "m4.png")
DrawRes(m5, "m5.png")
DrawRes(m6, "m6.png")
DrawRes(m7, "m7.png")
cv2.waitKey()

