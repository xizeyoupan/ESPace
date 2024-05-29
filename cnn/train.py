import os
from time import time

import torch
import torch.nn as nn
import torch.optim as optim

from torchvision import datasets
from torch.utils.data import DataLoader, WeightedRandomSampler

from torchvision.transforms import ToTensor

import matplotlib.pyplot as plt

from util import CURRENTPATH
from model import MyNetwork
from dataset import MyDataset

EPOCHS = 5000
info = [[], [], [], [], []]


def train(dataloader, device, model, loss_fn, optimizer, epoch_times):
    running_loss = 0.0
    correct = 0
    total = 0
    for batch, (inputs, labels) in enumerate(dataloader):
        model.train()
        inputs = inputs.to(device)
        labels = labels.to(device)
        optimizer.zero_grad()
        outputs = model(inputs)
        loss = loss_fn(outputs, labels)
        loss.backward()
        optimizer.step()
        running_loss += loss.item()
        _, predicted = torch.max(outputs.data, 1)
        total += labels.size(0)
        correct += (predicted == labels).sum().item()
    total_loss = running_loss / len(dataloader)
    acc = 100.0 * correct / total

    print(f"Epoch: {epoch_times:>5}, total loss: {total_loss:>0.3f}")
    info[0].append(epoch_times)
    info[1].append(acc)
    info[2].append(total_loss)


def test(dataloader, device, model, loss_fn):
    correct = 0
    total = 0
    running_loss = 0.0
    with torch.no_grad():
        for inputs, labels in dataloader:
            model.eval()
            inputs = inputs.to(device)
            labels = labels.to(device)
            outputs = model(inputs)
            loss = loss_fn(outputs, labels)
            running_loss += loss.item()
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()

        acc = 100.0 * correct / total
        val_loss = running_loss / len(dataloader)
        print(f"val loss: {val_loss:>0.3f}, accuracy: {acc:>0.6f} %")
        info[3].append(acc)
        info[4].append(val_loss)


def main():
    data_path = os.path.join(CURRENTPATH)

    print("Loading training data...")
    train_data = MyDataset(os.path.join(data_path, "data_train.csv"))

    print("Loading test data...")

    test_data = MyDataset(os.path.join(data_path, "data_test.csv"))

    train_dataloader = DataLoader(train_data, batch_size=256, shuffle=True)

    test_dataloader = DataLoader(test_data, batch_size=256, shuffle=True)

    device = "cuda" if torch.cuda.is_available() else "cpu"
    print(f"Using {device}")
    model = MyNetwork().to(device)
    print(model)

    loss_fn = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    epochs = EPOCHS

    for t in range(epochs):
        start_time = time()
        print(f"epoch {t+1} / {epochs}\n--------------------")
        train(train_dataloader, device, model, loss_fn, optimizer, t + 1)
        test(test_dataloader, device, model, loss_fn)
        end_time = time()
        print(f"time: {end_time-start_time:>0.2f} seconds")

        plt.plot(info[0], info[1], "o-", label="train acc")
        plt.plot(info[0], info[3], "o-", label="test acc")
        plt.xlabel("epochs")
        plt.ylabel("acc")
        plt.legend()

        # plt.show(block=False)

        plt.plot(info[0], info[4], "o-", label="val loss")
        plt.plot(info[0], info[2], "o-", label="total loss")
        plt.xlabel("epochs")
        plt.ylabel("loss")
        plt.legend()
        plt.pause(0.1)
        plt.show(block=False)
        plt.clf()

        if info[3][-1] == max(info[3]) and info[3][-1] > 98:
            path = os.path.join(CURRENTPATH, f"epoch-{info[0][-1]}-acc-{info[3][-1]}-model.pt")
            model.eval()
            torch.save(
                {
                    "epoch": t + 1,
                    "model_state_dict": model.state_dict(),
                    "optimizer_state_dict": optimizer.state_dict(),
                },
                path,
            )

            dummy_input = torch.rand(1, 6, 50)

            torch.onnx.export(model,         # model being run
                              dummy_input,       # model input (or a tuple for multiple inputs)
                              os.path.join(CURRENTPATH, f"epoch-{info[0][-1]}-acc-{info[3][-1]}-model.onnx"),       # where to save the model
                              export_params=True,  # store the trained parameter weights inside the model file
                              opset_version=14,    # the ONNX version to export the model to
                              do_constant_folding=True,  # whether to execute constant folding for optimization
                              input_names=['input'],   # the model's input names
                              output_names=['output'],  # the model's output names
                              dynamic_axes={'input': {0: 'batch_size'},    # variable length axes
                                            'output': {0: 'batch_size'}})

            print(f"model saved: {path}")
            exit()


if __name__ == "__main__":
    main()
