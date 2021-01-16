import cv2
from torch.autograd import Variable
import torch

#tens = Variable(torch.HalfTensor(8, 8).cuda().uniform_(-0.1, 0.1), requires_grad=True)
#
#mat = tens.detach().cpu().float().numpy()
#cv2.imwrite("sas.exr", mat, [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_FLOAT])

img = cv2.imread("W1.pt.exr", cv2.IMREAD_UNCHANGED)
tens = torch.load("W1.pt.pt")

print(img)
print(tens)

cv2.imwrite("sas.exr", img, [cv2.IMWRITE_EXR_TYPE, cv2.IMWRITE_EXR_TYPE_HALF])
img = cv2.imread("sas.exr", cv2.IMREAD_UNCHANGED)
print(img)
