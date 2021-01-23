import torch
import cv2
import math
import imageio
from torch.autograd import Variable

GI_RESOLUTION = 128
RAYS_PER_AXIS = 16
FC_RESOLUTION = 8

TRAINING_PICTURES = 32
TRAINING_SAMPLES = 32
BATCH_SIZE_GPU = 32

PICTURES_IN_RAM = 32 # Max = 64?
PICTURES_IN_RAM_REUSE = 1

TRAIN = (TRAINING_PICTURES - 1) * TRAINING_SAMPLES
TEST = 0
LOAD = 0

neuronsCount_L1 = 1024

conv1Size = 4
conv2Size = 4
conv1Kernels = 8
conv2Kernels = 4

alpha = 0.02
beta2 = 0.001
tanhMul = 1.2
rng = 0.005
steps = 4096
l2regMul = 0.001

TOTAL_COUNT = TRAINING_SAMPLES * TRAINING_PICTURES
BATCHES = TRAIN * (1 - TEST) + (TRAINING_SAMPLES * TRAINING_PICTURES - TRAIN) * TEST
PICTURES_FOR_TEST = 1 * TEST + (1 - TEST) * PICTURES_IN_RAM

learn = 1 - TEST
acc = 0.5
pr = 16

if TEST == 0 and LOAD == 1:
    alpha = 0.004
    beta2 = 0.0001

beta1 = alpha
pr = pr * (1 - TEST) + 1 * TEST
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


# Should I normalize data from [0; 1] to [-1; 1] ?
#def inpInit(i):
#    x = (i % NN_BATCHES) / (NN_BATCHES - 1)
#    y = ((i // NN_BATCHES) % NN_BATCHES) / (NN_BATCHES - 1)
#    z = (i // NN_BATCHES ** 2) / (NN_BATCHES - 1)
#    return [x, y, z, 1.0]


#inputsXYZ = torch.FloatTensor([[inpInit(i)] for i in range(NN_BATCHES**3)]).resize(1, NN_BATCHES**3, 1, 4) * 2 - 1

#data_x_XYZ = Variable(inputsXYZ.cuda())

torch.manual_seed(4221)

WEIGHTS1 = Variable(torch.FloatTensor(RAYS_PER_AXIS**3, neuronsCount_L1).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS1 = Variable(torch.FloatTensor(1, neuronsCount_L1).cuda().uniform_(-rng, rng), requires_grad=True)

#WEIGHTS2 = Variable(torch.FloatTensor(neuronsCount_L1, neuronsCount_L2).cuda().uniform_(-rng, rng), requires_grad=True)
#BIAS2 = Variable(torch.FloatTensor(1, neuronsCount_L2).cuda().uniform_(-rng, rng), requires_grad=True)

WEIGHTS2 = Variable(torch.FloatTensor(neuronsCount_L1, FC_RESOLUTION ** 3).cuda().uniform_(-rng, rng), requires_grad=True)
BIAS2 = Variable(torch.FloatTensor(1, FC_RESOLUTION ** 3).cuda().uniform_(-rng, rng), requires_grad=True)

CONV_W1 = Variable(torch.FloatTensor(1,conv1Kernels,conv1Size,conv1Size,conv1Size).cuda().uniform_(-rng, rng), requires_grad=True)
CONV_B1 = Variable(torch.FloatTensor(conv1Kernels).cuda().uniform_(-rng, rng), requires_grad=True)

CONV_W2 = Variable(torch.FloatTensor(1,conv2Kernels,conv2Size,conv2Size,conv2Size).cuda().uniform_(-rng, rng), requires_grad=True)
CONV_B2 = Variable(torch.FloatTensor(conv2Kernels).cuda().uniform_(-rng, rng), requires_grad=True)


if LOAD == 1 or TEST == 1:
    WEIGHTS1 = Variable(torch.load("W1.pt").cuda(), requires_grad=True)
    BIAS1 = Variable(torch.load("B1.pt").cuda(), requires_grad=True)
    WEIGHTS2 = Variable(torch.load("W2.pt").cuda(), requires_grad=True)
    BIAS2 = Variable(torch.load("B2.pt").cuda(), requires_grad=True)

    CONV_W1 = Variable(torch.load("CW1.pt").reshape(CONV_W1.shape).cuda(), requires_grad=True)
    CONV_B1 = Variable(torch.load("CB1.pt").reshape(CONV_B1.shape).cuda(), requires_grad=True)
    CONV_W2 = Variable(torch.load("CW2.pt").reshape(CONV_W2.shape).cuda(), requires_grad=True)
    CONV_B2 = Variable(torch.load("CB2.pt").reshape(CONV_B2.shape).cuda(), requires_grad=True)
    #WEIGHTS3 = Variable(torch.load("W3.pt").cuda(), requires_grad=True)
    #BIAS3 = Variable(torch.load("B3.pt").cuda(), requires_grad=True)


def fc_rrelu(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.rrelu_(h1)

    return a1


def fc_relu(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.relu_(h1)

    return a1


def fc_tanh(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.tanh_(h1)

    return a1

def fc_sigm(inp, weights, bias):
    h1 = inp @ weights + bias
    a1 = torch.sigmoid_(h1)

    return a1


def fc(inp, weights, bias):
    h1 = inp @ weights + bias

    return h1


def convTr(inp, weights, bias, stride):
    c1 = torch.conv_transpose3d(inp, weights, bias, stride=stride)
    c1Pull = (torch.max((c1),dim=1, keepdim=True)[0])

    return c1Pull


def Grad(tens, name, ppr, addGrad):
    norm = 0
    for t in tens:
        grad = t.grad.data
        norm += torch.sum(grad ** 2)
    norm = torch.sqrt(norm)
    for u in range(len(tens)):
        tk = tens[u]
        grad = tk.grad.data
        grad = grad / torch.sqrt(torch.sum(grad**2)) #norm #
        if True:
            tk.data.add_(grad.mul(-alpha * learn))
            ret = torch.sum(torch.abs(tk.grad.data))
            tk.grad.data.zero_()
        if ppr:
            print(name[u] + ": " + str(ret))

    return


def Forward(data_inp, weights, biases, batches): #, w3_2, b3_2
    n1 = fc_tanh(data_inp, weights[0], biases[0])
    y = fc_tanh(n1, weights[1], biases[1])
    y = y.resize(batches, 1, FC_RESOLUTION, FC_RESOLUTION, FC_RESOLUTION)
    c1 = torch.tanh(convTr(y, weights[2], biases[2], conv1Size))
    c2 = convTr(c1, weights[3], biases[3], conv2Size)
    return torch.tanh(c2)


def Func(inputs, batches):
    nn = Forward(inputs, [WEIGHTS1, WEIGHTS2, CONV_W1, CONV_W2], [BIAS1, BIAS2, CONV_B1, CONV_B2], batches) #, WEIGHTS3_2, BIAS3_2
    #y = nn * data_x_XYZ
    #Y = torch.tanh(torch.sum(y, 3)) * 1.05
    return nn


def Train(counter, ppr):
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
    Y = Func(data_x_Dist, BATCH_SIZE_GPU)
    loss = torch.mean((Y - data_y_GI) ** 2)
    #l2reg = ((torch.norm(WEIGHTS1) + torch.norm(BIAS1)) / neuronsCount_L1 +
    #         (torch.norm(WEIGHTS2) + torch.norm(BIAS2)) / neuronsCount_L1 +
    #         (torch.norm(WEIGHTS3) + torch.norm(BIAS3)) / neuronsCount_L2
    #         ) * l2regMul
    F = loss# + l2reg
    F.backward()

    #if :
    Grad([WEIGHTS1, WEIGHTS2, CONV_W1, CONV_W2, BIAS1, BIAS2, CONV_B1, CONV_B2], ["W1", "W2", "CW1", "CW2", "B1", "B2", "CB1", "CB2"], ppr, counter % (TRAIN // BATCH_SIZE_GPU) == 0)

    return loss


def lerp(x, y, alpha):
    l = y * alpha + x * (1 - alpha)
    return l


globalLoss = 0
glLossCount = 0


rangeBatch = steps * (1-TEST)
if (TEST == 1):
    rangeBatch = BATCHES // BATCH_SIZE_GPU
for i in range(rangeBatch):
    ppr = i % pr == 0 or i == steps - 1 or TEST == 1
    globalLoss += Train(i + TEST * TRAIN, ppr)
    glLossCount += 1

    alpha = lerp(alpha, lerp(beta1, beta2, math.tanh(i / steps * tanhMul)), 1 - acc)
    if ppr:
        print("StdDev : " + str(globalLoss / glLossCount / 2)) # Due to [-1;+1] to retail [0;+1] renormalization
        print("LR : " + str(alpha) + "; Step : " + str(i))
        globalLoss = 0
        glLossCount = 0



def DrawRes(i, name):
    data_x_Dist = Variable(inputs.float().cuda())
    data_y_GI = Variable(outputs.float().cuda())
    res = data_x_Dist[i, :].resize(1, RAYS_PER_AXIS**3)
    y = Func(res, 1)
    #t1 = y.resize(NN_BATCHES ** 3, NN_RESOLUTION ** 3).detach().cpu().float().numpy()
    #cv2.imwrite(name, t1 * 255, [cv2.IMWRITE_PAM_FORMAT_GRAYSCALE])
    #t2 = torch.abs(data_y_GI[i, :, :] - y).resize(NN_BATCHES ** 3, FC_RESOLUTION ** 3).detach().cpu().float().numpy()
    #cv2.imwrite("diff_" + name, t2 * 255, [cv2.IMWRITE_PAM_FORMAT_GRAYSCALE])
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

    Save(CONV_W1.resize(conv1Kernels, conv1Size**3), "CW1")
    Save(CONV_W2.resize(conv2Kernels, conv2Size**3, 1), "CW2")
    Save(CONV_B1.resize(conv1Kernels), "CB1")
    Save(CONV_B2.resize(conv2Kernels, 1), "CB2")


if TEST == 2:
    for i in range(BATCHES):
        DrawRes(i, "Dist" + str(i + TEST * TRAIN) + ".png")
