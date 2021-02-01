import cv2
import torch
import numpy
GI_RESOLUTION = 128

imgRT = cv2.imread("training\\RT0.exr", cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)[:, :, 2]
img = cv2.imread("training\\GI0.exr", cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)[:, :, 2]

img3d = torch.empty(GI_RESOLUTION, GI_RESOLUTION, GI_RESOLUTION)

for i in range(GI_RESOLUTION):
    img3d[:, :, i] = torch.from_numpy(img[0:GI_RESOLUTION,i*GI_RESOLUTION:(i+1)*GI_RESOLUTION]).transpose(0, 1)

out = img3d.numpy()
fft = numpy.fft.fftn(img3d.numpy(), s=[GI_RESOLUTION, GI_RESOLUTION, GI_RESOLUTION], axes=(0, 1, 2))
ifft = numpy.fft.ifftn(fft, s=[GI_RESOLUTION, GI_RESOLUTION, GI_RESOLUTION], axes=(0, 1, 2)).real

out = ifft[:, :, 0]
cv2.imshow("A", out.T)
cv2.waitKey(0)

