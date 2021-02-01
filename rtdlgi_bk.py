import torch
import cv2
import math
from collections import OrderedDict
from torch.autograd import Variable

GI_RESOLUTION = 128
RAYS_PER_AXIS = 16
NN_RESOLUTION = 8

TRAINING_PICTURES = 32
TRAINING_SAMPLES = 32
BATCH_SIZE_GPU = 32

PICTURES_IN_RAM_REUSE = 1

TRAIN = (TRAINING_PICTURES - 1) * TRAINING_SAMPLES
TEST = 0
LOAD = 0

neuronsCount_L1 = 256

conv1Size = 5
conv1Kernels = 3

lr1 = 0.001
lr2 = 0.00001

steps = 4096

PICTURES_IN_RAM = min(TRAINING_PICTURES, 64) # Max = 64?
TOTAL_COUNT = TRAINING_SAMPLES * TRAINING_PICTURES
BATCHES = TRAIN * (1 - TEST) + (TRAINING_SAMPLES * TRAINING_PICTURES - TRAIN) * TEST
PICTURES_FOR_TEST = 1 * TEST + (1 - TEST) * PICTURES_IN_RAM

pr = 16

pr = pr * (1 - TEST) + 1 * TEST
LOAD = max(LOAD,TEST)
# training data
TRAINING_SAMPLES_IN_RAM = TRAINING_SAMPLES * (PICTURES_IN_RAM - 1) * (1 - TEST) + TRAINING_SAMPLES * TEST
inputs = torch.empty(TRAINING_SAMPLES_IN_RAM, RAYS_PER_AXIS ** 3)
outputs = torch.empty(TRAINING_SAMPLES_IN_RAM, GI_RESOLUTION ** 3)

def LoadData(iteration):
    for i in range((PICTURES_IN_RAM - 1) * (1 - TEST) + 1 * TEST):
        imgOrgDist = cv2.imread("training\\RT" + str(i * (1 - TEST) + (PICTURES_IN_RAM - 1) * TEST % TRAINING_PICTURES) + ".exr", cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)[:, :, 2]
        imgOrgGI = cv2.imread("training\\GI" + str(i * (1 - TEST) + (PICTURES_IN_RAM - 1) * TEST % TRAINING_PICTURES) + ".exr", cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)[:, :, 2]
        print("Loading *" + str(i) + ".exr")
        for j in range(TRAINING_SAMPLES):
            POS = j + i * TRAINING_SAMPLES
            distBatch = j * RAYS_PER_AXIS
            imgDist = torch.from_numpy(imgOrgDist[distBatch:distBatch+RAYS_PER_AXIS, :])
            inputs[POS, :] = imgDist.resize(RAYS_PER_AXIS ** 3)

            giBatch = j * GI_RESOLUTION
            imgGI = torch.from_numpy(imgOrgGI[giBatch:giBatch+GI_RESOLUTION,:]) * 2.0 - 1.0
            outputs[POS, :] = imgGI.resize(GI_RESOLUTION ** 3)


modelNN1 = torch.nn.Sequential(OrderedDict([
    ('l1', torch.nn.Linear(RAYS_PER_AXIS**3, neuronsCount_L1)),
    ('n1', torch.nn.ReLU()),
    ('l2', torch.nn.Linear(neuronsCount_L1, NN_RESOLUTION ** 3)),
    ('n2', torch.nn.ReLU()),
])).float().cuda()


modelNN2 = torch.nn.Sequential(OrderedDict([
    ('u1', torch.nn.Upsample(scale_factor=4, mode='trilinear')),
    ('c1', torch.nn.Conv3d(in_channels = 1, out_channels = conv1Kernels, kernel_size = conv1Size, padding = (conv1Size - 1) // 2)),
    ('n1', torch.nn.ReLU()),
    ('u2', torch.nn.Upsample(scale_factor=4, mode='trilinear')),
    ('c2', torch.nn.Conv3d(in_channels = conv1Kernels, out_channels = 1, kernel_size = conv1Size, padding = (conv1Size - 1) // 2)),
    ('n2', torch.nn.Tanh())
])).float().cuda()

if LOAD == 0:
    modelNN1.l1.weight = torch.nn.init.xavier_uniform_(modelNN1.l1.weight, 5/3)
    modelNN1.l2.weight = torch.nn.init.xavier_uniform_(modelNN1.l2.weight, 5/3)
    modelNN2.c1.weight = torch.nn.init.xavier_uniform_(modelNN2.c1.weight, 1)
    modelNN2.c2.weight = torch.nn.init.xavier_uniform_(modelNN2.c2.weight, 1)
else:
    modelNN1.l1.weight.data = torch.load("out\\W1.pt")
    modelNN1.l1.bias.data = torch.load("out\\B1.pt")
    modelNN1.l2.weight.data = torch.load("out\\W2.pt")
    modelNN1.l2.bias.data = torch.load("out\\B2.pt")
    modelNN2.c1.weight.data = torch.load("out\\CW1.pt").reshape(conv1Kernels, 1, conv1Size, conv1Size, conv1Size)
    modelNN2.c1.bias.data = torch.load("out\\CB1.pt").reshape(conv1Kernels)
    modelNN2.c2.weight.data = torch.load("out\\CW2.pt").reshape(1, conv1Kernels, conv1Size, conv1Size, conv1Size)
    modelNN2.c2.bias.data = torch.load("out\\CB2.pt").reshape(1)

def Forward(data_inp): #, w3_2, b3_2
    out1 = modelNN1(data_inp)
    out1RS = torch.reshape(out1, (BATCH_SIZE_GPU, 1, NN_RESOLUTION, NN_RESOLUTION, NN_RESOLUTION))
    out2 = modelNN2(out1RS)
    return out2


def Train(counter, lr, ppr):
    startIdx = (counter * BATCH_SIZE_GPU) % (BATCHES)
    endIdx = ((counter + 1) * BATCH_SIZE_GPU) % (BATCHES)
    if endIdx == 0:
        endIdx = BATCHES
    if (endIdx - startIdx < 0):
        raise 1

    if ((counter == 0 or TEST == 1) or ((PICTURES_IN_RAM != TRAINING_PICTURES) and (counter % (PICTURES_IN_RAM // (TRAINING_SAMPLES // BATCH_SIZE_GPU))) == 0)):
        LoadData(counter // (PICTURES_IN_RAM // (TRAINING_SAMPLES // BATCH_SIZE_GPU)))
    POS_START = startIdx % TRAINING_SAMPLES_IN_RAM
    POS_END = endIdx % TRAINING_SAMPLES_IN_RAM
    if POS_END == 0:
        POS_END = TRAINING_SAMPLES_IN_RAM
    data_x_Dist = Variable(inputs[POS_START:POS_END, :].float().cuda())
    data_y_GI = Variable(outputs[POS_START:POS_END, :].float().cuda().resize(BATCH_SIZE_GPU, 1, GI_RESOLUTION, GI_RESOLUTION, GI_RESOLUTION))

    Y = Forward(data_x_Dist)
    loss = torch.mean((Y - data_y_GI) ** 2)
    loss.backward()
    if ppr:
        print("FC1 W: " + str(torch.sum(torch.abs(modelNN1.l1.weight)).item()) + " B: " + str(torch.sum(torch.abs(modelNN1.l1.bias)).item()))
        print("FC2 W: " + str(torch.sum(torch.abs(modelNN1.l2.weight)).item()) + " B: " + str(torch.sum(torch.abs(modelNN1.l2.bias)).item()))
        print("Conv1 W: " + str(torch.sum(torch.abs(modelNN2.c1.weight)).item()) + " B: " + str(torch.sum(torch.abs(modelNN2.c1.bias)).item()))
        print("Conv2 W: " + str(torch.sum(torch.abs(modelNN2.c2.weight)).item()) + " B: " + str(torch.sum(torch.abs(modelNN2.c2.bias)).item()))

    optimizer1 = torch.optim.Adam(modelNN1.parameters(), lr=lr)
    optimizer2 = torch.optim.Adam(modelNN2.parameters(), lr=lr/10)

    optimizer1.step()
    optimizer2.step()

    optimizer1.zero_grad()
    optimizer2.zero_grad()

    return loss


def lerp(x, y, alpha):
    l = y * (1 - alpha) + x * alpha
    return l


globalLoss = 0
glLossCount = 0


rangeBatch = steps * (1-TEST)
if (TEST == 1):
    rangeBatch = BATCHES // BATCH_SIZE_GPU
for i in range(rangeBatch):
    ppr = i % pr == 0 or i == steps - 1 or TEST == 1
    lr = lerp(lr1, lr2, 1 - math.tanh(1 - rangeBatch / (i * rangeBatch / 10.0 * 1.2 + rangeBatch)))
    loss = Train(i + TEST * TRAIN, lr, ppr)

    #alpha = lerp(alpha, lerp(beta1, beta2, math.tanh(i / steps * tanhMul)), 1 - acc)
    if ppr:
        print("StdDev : " + str(loss.item() / 2)) # Due to [-1;+1] to retail [0;+1] renormalization
        print("LR : " + str(lr) + "; Step : " + str(i))


def Save(tens, name):
    torch.save(tens, "out\\" + name + ".pt")
    if (tens.ndim == 2):
        tens = torch.transpose(tens, 0, 1).flatten()
    mat = tens.view(-1).detach().cpu().float().numpy()
    cv2.imwrite("out\\" + name + ".exr", mat, [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_FLOAT])

if TEST == 0:
    Save(modelNN1.l1.weight, "W1")
    Save(modelNN1.l1.bias, "B1")

    Save(modelNN1.l2.weight, "W2")
    Save(modelNN1.l2.bias, "B2")

    Save(modelNN2.c1.weight, "CW1")
    Save(modelNN2.c1.bias, "CB1")
    Save(modelNN2.c2.weight, "CW2")
    Save(modelNN2.c2.bias, "CB2")

