import torch
import cv2
import math
import imageio
from torch.autograd import Variable

GI_RESOLUTION = 64
RAYS_PER_AXIS = 16
TRAINING_SAMPLES = 32
NN_RESOLUTION = 8
TRAINING_BATCHES = 256
BATCH_SIZE = 256

TRAIN = 7936
TEST = 1
LOAD = 0

neuronsCount_L1 = 2048
neuronsCount_L2 = 2048
neuronsCount_L3 = 2048
neuronsCount_L4 = 2048

alpha = 0.04
beta2 = 0.001
tanhMul = 1.2
rng = 0.09
steps = 10000
l2regMul = 10.0

NN_BATCHES = GI_RESOLUTION // NN_RESOLUTION
BATCHES = TRAIN * (1 - TEST) + (TRAINING_SAMPLES * TRAINING_BATCHES - TRAIN) * TEST

learn = 1 - TEST
beta1 = alpha
acc = 0.5
pr = 100


pr = pr * (1 - TEST) + 1 * TEST
# training data
inputs = torch.zeros(BATCHES, RAYS_PER_AXIS**3).half()
outputs = torch.zeros(BATCHES, NN_BATCHES ** 3, NN_RESOLUTION ** 3).half()

NN_RESOLUTION3 = NN_RESOLUTION**3
def LoadData():
    for i in range(TRAINING_BATCHES):
        imgOrgDist = imageio.imread("training\\RT" + str(i) + ".dds")[:, :, 2]
        imgOrgGI = imageio.imread("training\\GI" + str(i) + ".dds")[:, :, 2]
        for j in range(TRAINING_SAMPLES):
            localPos = i * TRAINING_SAMPLES + j
            if (TEST == 1) and localPos < TRAIN:
                continue
            if (localPos == TRAIN) and (TEST == 0):
                return
            POS = localPos - TEST * TRAIN

            distBatch = j * RAYS_PER_AXIS
            imgDist = torch.from_numpy(imgOrgDist[distBatch:distBatch+RAYS_PER_AXIS, :]) / 255.0
            inputs[POS, :] = imgDist.resize(RAYS_PER_AXIS ** 3).half()

            giBatch = j * NN_RESOLUTION3
            imgGI = torch.from_numpy(imgOrgGI[giBatch:giBatch+NN_RESOLUTION3, :]) / 255.0 * 2.0 - 1.0
            outputs[POS, :, :] = imgGI.resize(NN_BATCHES ** 3, NN_RESOLUTION3).half()


LoadData()

# Should I normalize data from [0; 1] to [-1; 1] ?
def inpInit(i):
    x = (i % NN_BATCHES) / (NN_BATCHES - 1)
    y = ((i // NN_BATCHES) % NN_BATCHES) / (NN_BATCHES - 1)
    z = (i // NN_BATCHES ** 2) / (NN_BATCHES - 1)
    return [x, y, z, 1.0]


inputsXYZ = torch.FloatTensor([[inpInit(i)] for i in range(NN_BATCHES**3)]).half().resize(1, NN_BATCHES**3, 1, 4) * 2 - 1

data_x_XYZ = Variable(inputsXYZ.float().cuda())

torch.manual_seed(4221)

WEIGHTS1 = Variable(torch.FloatTensor(RAYS_PER_AXIS**3, neuronsCount_L1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1 = Variable(torch.FloatTensor(1, neuronsCount_L1).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS2 = Variable(torch.FloatTensor(neuronsCount_L1, neuronsCount_L2).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2 = Variable(torch.FloatTensor(1, neuronsCount_L2).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS3 = Variable(torch.FloatTensor(neuronsCount_L2, neuronsCount_L3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS3 = Variable(torch.FloatTensor(1, neuronsCount_L3).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS4 = Variable(torch.FloatTensor(neuronsCount_L3, neuronsCount_L4).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS4 = Variable(torch.FloatTensor(1, neuronsCount_L4).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS5 = Variable(torch.FloatTensor(neuronsCount_L4, NN_RESOLUTION**3 * 4).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS5 = Variable(torch.FloatTensor(1, NN_RESOLUTION**3 * 4).cuda().uniform_(-rng, rng), requires_grad=True)


if LOAD == 1 or TEST == 1:
    WEIGHTS1 = Variable(torch.load("W1.pt").cuda(), requires_grad=True)
    BIAS1 = Variable(torch.load("B1.pt").cuda(), requires_grad=True)
    WEIGHTS2 = Variable(torch.load("W2.pt").cuda(), requires_grad=True)
    BIAS2 = Variable(torch.load("B2.pt").cuda(), requires_grad=True)
    WEIGHTS3 = Variable(torch.load("W3.pt").cuda(), requires_grad=True)
    BIAS3 = Variable(torch.load("B3.pt").cuda(), requires_grad=True)
    WEIGHTS4 = Variable(torch.load("W4.pt").cuda(), requires_grad=True)
    BIAS4 = Variable(torch.load("B4.pt").cuda(), requires_grad=True)
    WEIGHTS5 = Variable(torch.load("W5.pt").cuda(), requires_grad=True)
    BIAS5 = Variable(torch.load("B5.pt").cuda(), requires_grad=True)


def fc_rrelu(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.rrelu(h1)

    return a1

def fc_tanh(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.tanh(h1)

    return a1

def fc(inp, weights, bias):
    h1 = inp @ weights + bias

    return h1



def Grad(s):
    grad = s.grad.data
    grad = grad / torch.sqrt(torch.sum(grad ** 2))
    s.data.add_(grad.mul(-alpha*learn))
    ret = torch.sum(torch.abs(s.grad.data))
    s.grad.data.zero_()

    return ret


def Forward(data_inp, weights, biases): #, w3_2, b3_2
    n1 = fc_tanh(data_inp, weights[0], biases[0])
    n2 = fc_tanh(n1, weights[1], biases[1])
    n3 = fc_tanh(n2, weights[2], biases[2])
    n4 = fc_tanh(n3, weights[3], biases[3])
    y = fc(n4, weights[4], biases[4])
    return [y] #[y, y2]


def Func(inputs, batches):
    nn = Forward(inputs, [WEIGHTS1, WEIGHTS2, WEIGHTS3, WEIGHTS4, WEIGHTS5], [BIAS1, BIAS2, BIAS3, BIAS4, BIAS5]) #, WEIGHTS3_2, BIAS3_2
    NN1 = nn[0].resize(batches, 1, NN_RESOLUTION**3, 4)
    y = NN1 * data_x_XYZ
    Y = torch.tanh(torch.sum(y, 3))

    return Y

def Train(counter, ppr):
    startIdx = (counter * BATCH_SIZE) % (BATCHES)
    endIdx = ((counter + 1) * BATCH_SIZE) % (BATCHES)
    if endIdx == 0:
        endIdx = BATCHES
    if (endIdx - startIdx < 0):
        raise 1

    data_x_Dist = Variable(inputs[startIdx:endIdx, :].float().cuda())
    data_y_GI = Variable(outputs[startIdx:endIdx, :, :].float().cuda())
    Y = Func(data_x_Dist, endIdx - startIdx)
    loss = torch.mean((Y - data_y_GI) ** 2)
    l2reg = ((torch.norm(WEIGHTS1) + torch.norm(BIAS1)) / neuronsCount_L1 +
             (torch.norm(WEIGHTS2) + torch.norm(BIAS2)) / neuronsCount_L1 +
             (torch.norm(WEIGHTS3) + torch.norm(BIAS3)) / neuronsCount_L2 +
             (torch.norm(WEIGHTS4) + torch.norm(BIAS4)) / neuronsCount_L3 +
             (torch.norm(WEIGHTS5) + torch.norm(BIAS5)) / neuronsCount_L4
             ) * l2regMul
    F = loss + l2reg
    F.backward()

    gr = Grad(WEIGHTS1)
    gr += Grad(BIAS1)
    gr += Grad(WEIGHTS2)
    gr += Grad(BIAS2)
    gr += Grad(WEIGHTS3)
    gr += Grad(BIAS3)
    gr += Grad(WEIGHTS4)
    gr += Grad(BIAS4)
    gr += Grad(WEIGHTS5)
    gr += Grad(BIAS5)
    #gr += Grad(WEIGHTS3_2)
    #gr += Grad(BIAS3_2)

    if ppr:
        print("Grad: " + str(gr))
    return loss


def lerp(x, y, alpha):
    l = y * alpha + x * (1 - alpha)
    return l


counter = 0
rangeBatch = steps * (1-TEST)
if (TEST == 1):
    rangeBatch = BATCHES // BATCH_SIZE
for i in range(rangeBatch):
    ppr = i % pr == 0 or i == steps - 1 or i < (pr / 10) or TEST == 1
    loss = Train(counter, ppr)
    counter += 1

    alpha = lerp(alpha, lerp(beta1, beta2, math.tanh(i / steps * tanhMul)), 1 - acc)
    if ppr:
        print("StdDev : " + str(loss / 2)) # Due to [-1;+1] to retail [0;+1] renormalization
        print("LR : " + str(alpha) + "; Gen : " + str(i))


def DrawRes(i, name):
    data_x_Dist = Variable(inputs.float().cuda())
    data_y_GI = Variable(outputs.float().cuda())
    res = data_x_Dist[i, :].resize(1, RAYS_PER_AXIS**3)
    y = Func(res, 1)
    #t1 = y.resize(NN_BATCHES ** 3, NN_RESOLUTION ** 3).detach().cpu().float().numpy()
    #cv2.imwrite(name, t1 * 255, [cv2.IMWRITE_PAM_FORMAT_GRAYSCALE])
    t2 = torch.abs(data_y_GI[i, :, :] - y).resize(NN_BATCHES ** 3, NN_RESOLUTION ** 3).detach().cpu().float().numpy()
    cv2.imwrite("diff_" + name, t2 * 255, [cv2.IMWRITE_PAM_FORMAT_GRAYSCALE])
    #cv2.imshow(name, t1)


def Save(tens, name):
    torch.save(tens, name + ".pt")
    mat = tens.detach().cpu().float().numpy()
    cv2.imwrite(name + ".exr", mat, [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_FLOAT])

if TEST == 0:
    Save(WEIGHTS1, "W1")
    Save(BIAS1, "B1")

    Save(WEIGHTS2, "W2")
    Save(BIAS2, "B2")

    Save(WEIGHTS3, "W3")
    Save(BIAS3, "B3")

    Save(WEIGHTS4, "W4")
    Save(BIAS4, "B4")

    Save(WEIGHTS5, "W5")
    Save(BIAS5, "B5")


if TEST == 2:
    for i in range(BATCHES):
        DrawRes(i, "Dist" + str(i + TEST * TRAIN) + ".png")
