import os
from torch.utils.data import Dataset, DataLoader
from util import CURRENTPATH
import numpy as np


class MyDataset(Dataset):
    def __init__(self, path, transform=None):
        self.transform = transform

        d = np.loadtxt(path, delimiter=',', dtype="str", skiprows=1)
        self.data = {}

        for item in d:
            feat = item[1].split(';')
            feat = [[int(k) for k in i.split()] for i in feat]
            feat = np.array(feat, dtype=np.float32)

            # print(item)
            # print(feat.shape, feat.dtype)
            index = int(item[0])
            label = int(item[-1])
            for (i, v) in enumerate(feat):
                # print(index, i, len(v))
                assert len(v) == 50
            self.data[index] = [feat, label]

    def __getitem__(self, index):
        fn, label = self.data[index]
        if self.transform is not None:
            fn = self.transform(fn)
        return fn, label

    def __len__(self):
        return len(self.data)


if __name__ == '__main__':
    d = MyDataset(os.path.join(CURRENTPATH, 'data_test.csv'))
