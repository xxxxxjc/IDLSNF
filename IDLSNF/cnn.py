from option import args
from model.dlsn import DLSN
from PIL import Image
from torch.autograd import Variable
from torchvision.utils import save_image
import torch
import torchvision.transforms as transforms

torch.manual_seed(args.seed)


def tensor_to_PIL(tensor):
    unloader = transforms.ToPILImage()
    image = tensor.cpu().clone()
    image = image.squeeze(0)
    image = unloader(image)
    return image


def main():
    torch.set_grad_enabled(False)
    device = torch.device("cuda")
    demo_imput_dir = 'E:\\xjc\\dateset\\test\\tensor_Temp0.png'
    PATH = 'E:\\xjc\\test\\DLSN-main\\experiment\\model_best32(1).pt'
    inputimage = Image.open(demo_imput_dir).convert('L')
    transf = transforms.ToTensor()

    inputimage = transf(inputimage)
    inputimage = Variable(torch.unsqueeze(inputimage, dim=0).float(), requires_grad=False)

    model = DLSN(args).to(device)
    model.load_state_dict(torch.load(PATH))
    inputimage = inputimage.to(device)

    chunk_count = 256
    outlist = []
    inputlist = inputimage.chunk(chunks=chunk_count, dim=3)
    # chunk_count = len(inputlist)
    for i in range(chunk_count):
        print(i)
        outlist.append(model(inputlist[i]))
    # 循环结束 张量拼接
    X1 = torch.cat(outlist, 3)
    save_image(X1, 'E:\\xjc\\dateset\\test\\tensor_Temp1.png')
    torch.set_grad_enabled(True)


if __name__ == '__main__':
    main()