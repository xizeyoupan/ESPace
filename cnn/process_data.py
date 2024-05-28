import re
import pandas as pd
import os
import json
import random
current_path = os.path.dirname(os.path.realpath(__file__))

dtrain = {"seq": [], "value": []}
dtest = {"seq": [], "value": []}

for root, dirs, files in os.walk(os.path.join(current_path, 'data')):
    for name in files:
        data_file_path = os.path.join(root, name)
        print(data_file_path)
        if not name.endswith('.json'):
            continue
        value = name.split('-')[0]

        f = open(data_file_path, 'r')
        data = f.read()

        data = re.findall(r'\{[\s\S]*?\}', data)
        ax = []
        ay = []
        az = []
        gx = []
        gy = []
        gz = []
        for item in data:

            json_data = json.loads(item)
            # print(json_data)

            ax.append(str(json_data['ax']))
            ay.append(str(json_data['ay']))
            az.append(str(json_data['az']))
            gx.append(str(json_data['gx']))
            gy.append(str(json_data['gy']))
            gz.append(str(json_data['gz']))

            if json_data['cnt'] == 49:
                s = ';'.join([' '.join(ax), ' '.join(ay), ' '.join(az), ' '.join(gx), ' '.join(gy), ' '.join(gz),])
                # print(s)
                if random.random() < 0.5:
                    dtrain['seq'].append(s)
                    dtrain['value'].append(value)
                else:
                    dtest['seq'].append(s)
                    dtest['value'].append(value)

                ax = []
                ay = []
                az = []
                gx = []
                gy = []
                gz = []

        data_df = pd.DataFrame(dtrain)
        data_df.to_csv(os.path.join(current_path, 'data_train.csv'))
        data_df = pd.DataFrame(dtest)
        data_df.to_csv(os.path.join(current_path, 'data_test.csv'))
