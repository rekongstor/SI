import cv2
from torch.autograd import Variable
import torch

tens = Variable(torch.HalfTensor(8, 8).cuda().uniform_(-0.1, 0.1), requires_grad=True)

mat = tens.detach().cpu().float().numpy()
cv2.imwrite("sas.exr", mat, [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_FLOAT])

