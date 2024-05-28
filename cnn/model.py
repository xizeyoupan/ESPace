import torch.nn as nn


class MyNetwork(nn.Module):

    def __init__(self):
        super().__init__()

        # input:(bitch_size, 6, 50), output:(bitch_size, 18, 25)
        self.conv1 = nn.Sequential(
            nn.Conv1d(6, 18, 3, 1, 1),
            nn.ReLU(inplace=True),
            nn.MaxPool1d(kernel_size=2),
        )

        # input:(bitch_size, 18, 25), output:(bitch_size, 36, 12)
        self.conv2 = nn.Sequential(
            nn.Conv1d(18, 36, 3, 1, 1),
            nn.ReLU(inplace=True),
            nn.MaxPool1d(kernel_size=2),
        )

        self.fc1 = nn.Sequential(
            # nn.Dropout(p=0.65),
            nn.Linear(in_features=36 * 12, out_features=512),
            nn.ReLU(inplace=True),
        )

        self.fc2 = nn.Sequential(
            # nn.Dropout(p=0.6),
            nn.Linear(in_features=512, out_features=128),
            nn.ReLU(inplace=True),
        )

        self.fc3 = nn.Linear(in_features=128, out_features=3)

    def forward(self, x):
        x = self.conv1(x)

        x = self.conv2(x)

        x = x.view(x.shape[0], -1)

        x = self.fc1(x)

        x = self.fc2(x)

        x = self.fc3(x)

        return x
