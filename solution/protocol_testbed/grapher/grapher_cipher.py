#!/usr/bin/env python3

import argparse, re, sys, math
import pandas as pd
from datetime import time, timedelta
import plotly.express as px
import plotly.graph_objects as go

DEBUG = True
RECORD_NUM  = 10923

out_basedir = "../out/"
server_out_basedir = "../out/server/"


def get_time_log(time_logfile_name_list):
    time_data_list = []
    for time_logfile_name in time_logfile_name_list:
        time_logfile = open(time_logfile_name, "r")
        time_data = {}
        for line in time_logfile:
            data = line.rstrip().split(",")
            time_data[data[1]] = time.fromisoformat(data[0])
        
        time_data_list.append(time_data)
        time_logfile.close()
    return time_data_list


def generate_cpu_usage_graph(top_logfile_name_list, time_data_list, cipher_name_list):
    
    top_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    
    #color_idx = 0
    #color = ['blue', 'red', 'orange']
    for top_logfile_name, time_data, cipher_name in zip(top_logfile_name_list, time_data_list, cipher_name_list):
        try:
            top_logfile = open(top_logfile_name, "r")
        except:
            print("top log is not found")
            return
        
        top_data = []
        for i, line in enumerate(top_logfile):
            line = line.rstrip("\n")
            line = line.lstrip()
            st = time_data["Program start"]
            gt = time_data["Records sent"]

            if re.search("^top - ", line):
                now = time.fromisoformat(re.split("\s+", line)[2])
                
            if re.search("protocol", line):
                row = re.split("\s+", line)
                row[8]= float(row[8])

                if now < st:
                    continue

                if now > gt:
                    break

                time_delta = timedelta(hours=now.hour - st.hour, minutes = now.minute - st.minute, seconds = now.second - st.second)
                row.append(time_delta.seconds)

                if len(row) != 13:
                    print("[top data] Format error", file=sys.stderr)
                    continue
                top_data.append(row)
        
        top_logfile.close()

        top_df = pd.DataFrame(top_data, columns = ["PID", "USER", "PR", "NI", "VIRT", "RES", "SHR", "S", "%CPU", "%MEM", "TIME+", "COMMAND", "TIME"])
        top_df = top_df.sort_values(by='%CPU', ascending=True)
        top_df["Rate"] = 0
        
        for i in range(len(top_df)):
            top_df.iloc[i, 13] =  (float(i + 1) / len(top_df)) * 100
       
        top_fig.add_trace(go.Scatter(x=top_df["Rate"], y = top_df["%CPU"], name=cipher_name))
        #top_fig.add_trace(go.Scatter(x=top_df["Rate"], y = top_df["%CPU"], name=cipher_name, line=dict(color=color[color_idx])))
        #color_idx = color_idx + 1

        x_min, x_max = 0, 101
        y_min, y_max = 0, 100

    top_fig.update_layout(
        xaxis = dict(
            title = "Percentage (%)",
            range = [x_min, x_max],
            dtick = 10
        ),
        yaxis = dict(
            title = "CPU usage (%)",
            range = [y_min, y_max],
            dtick = 20
        ),
        font=dict(
            family = "Times New Roman",
            size=36,
        ),
    )
    top_fig.update_traces(
        mode="lines+markers",
        marker=dict(
            size=4,
        ),
        line=dict(
            width=2,
        )
    )
    top_fig.show()

def generate_power_consumption_graph(power_logfile_name_list, time_data_list, cipher_name_list):
    
    power_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    for power_logfile_name, time_data, cipher_name in zip(power_logfile_name_list, time_data_list, cipher_name_list):
        try:
            power_logfile = open(power_logfile_name, "r")
        except:
            print("powerstat log is not found")
            return

        power_data = []
        #power_start_time = top_data["TIME"][0]
        #power_end_time = top_data["TIME"][-1]

        for line in power_logfile:
            line = line.rstrip()
            st = time_data["Program start"]
            gt = time_data["File sent"]
            
            if re.search("^[0-9]+:[0-9]+:[0-9]+", line):

                row = re.split("\s+", line)
                row[0] = time.fromisoformat(row[0])
                row[12] = float(row[12])

                if row[0] < st:
                    continue

                if row[0] > gt:
                    break

                time_delta = timedelta(hours=row[0].hour - st.hour, minutes = row[0].minute - st.minute, seconds = row[0].second - st.second)
                row[0] = time_delta.seconds
                
                if len(row) != 13:
                    print("[power data] Format error", file=sys.stderr)
                    continue
                power_data.append(row)
    
        power_logfile.close()

        power_df = pd.DataFrame(power_data, columns = ["Time", "User", "Nice", "Sys", "Idle", "IO", "Run", "Ctxt/s", "IRQ/s", "Fork", "Exec", "Exit", "Watts"])
        power_df = power_df.sort_values(by='Watts', ascending=True)
        power_df["Rate"] = 0

        for i in range(len(power_df)):
            power_df.iloc[i, 13] =  (float(i + 1) / len(power_df)) * 100
        
        power_fig.add_trace(go.Scatter(x=power_df["Rate"], y=power_df["Watts"], name=cipher_name))
    
        x_min, x_max = 0, 101
        y_min, y_max = min(min(power_df["Watts"]), y_min), max(max(power_df["Watts"]), y_max)


    if y_min - math.floor(y_min) < 0.5:
        y_min = math.floor(y_min)
    else:
        y_min = math.floor(y_min) + 0.5
    
    if y_max - math.floor(y_max) > 0.5:
        y_max = math.ceil(y_max)
    else:
        y_max = math.ceil(y_max) - 0.5

    power_fig.update_layout(
        xaxis = dict(
            title = "Percentage (%)",
            range = [x_min, x_max],
            dtick = 10
        ),
        yaxis = dict(
            title = "Discharge rate (W/s)",
            range = [y_min, y_max],
            dtick = 0.5
        ),
        font=dict(
            family = "Times New Roman",
            size=36,
        ),
    )
    power_fig.update_traces(
        mode="lines+markers",
        marker=dict(
            size=4,
        ),
        line=dict(
            width=4,
        )
    )
    power_fig.show()

    if DEBUG:
        print(power_df)


def generate_throughput_graph(throughput_logfile_name_list, cipher_name_list):
    
    throughput_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    for throughput_logfile_name, cipher_name in zip(throughput_logfile_name_list, cipher_name_list):
        throughput_logfile = open(throughput_logfile_name, "r")
        throughput_data = []

        data = ['']*2
        for line in throughput_logfile:
            line = line.rstrip("\n")
            line = line.lstrip()

            if re.search("^File transfer", line):

                row = re.split("\s+", line)
                data[0] = int(row[2].strip("%"))

            if re.search("^Speed", line):
                line = line.rstrip("\n")
                line = line.lstrip()

                row = re.split("\s+", line)
                data[1] = float(row[2])

                throughput_data.append(data)
                data = ['']*2
        
        throughput_logfile.close()

        throughput_df = pd.DataFrame(throughput_data, columns = ["transferred", "throughput"])
        throughput_df = throughput_df.sort_values(by='throughput', ascending=True)
        throughput_df["rate"] = 0

        for i in range(len(throughput_df)):
            throughput_df.iloc[i, 2] =  (float(i + 1) / len(throughput_df)) * 100
        
        if DEBUG:
            print(throughput_df)
    
        throughput_fig.add_trace(go.Scatter(x=throughput_df["rate"], y=throughput_df["throughput"], name=cipher_name))

        x_min, x_max = 0, 101
        y_min, y_max = min(math.floor(min(throughput_df["throughput"])), y_min) , max(math.ceil(max(throughput_df["throughput"])), y_max)

    if y_min % 2 != 0:
        y_min = y_min - 1
    if y_max % 2 != 0:
        y_max = y_max + 1

    throughput_fig.update_layout(
        xaxis = dict(
            title = "Percentage (%)",
            range = [x_min, x_max],
            dtick =10
        ),
        yaxis = dict(
            title = "Throughput (Mb/s)",
            range = [y_min, y_max],
            dtick = 4
        ),
        font=dict(
            family = "Times New Roman",
            size=36,
        ),
    )
    throughput_fig.update_traces(
        mode="lines+markers",
        marker=dict(
            size=4,
        ),
        line=dict(
            width=2,
        )
    )
    throughput_fig.show()

    if DEBUG:
        print(throughput_df)

def generate_latency_graph(throughput_logfile_name_list, cipher_name_list):
    
    latency_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    #color_idx = 0
    #color = ['blue', 'red', 'orange']
    for latency_logfile_name, cipher_name in zip(latency_logfile_name_list, cipher_name_list):
        latency_logfile = open(latency_logfile_name, "r")
        latency_data = []

        j = 0
        loss = 0
        for line in latency_logfile:
            line = line.rstrip("\n")
            line = line.lstrip()

            if re.search("^Id", line):
                line = line.rstrip("\n")
                line = line.lstrip()

                row = re.split("\s+", line)
                data = [int(row[1].strip('\"')), float(row[3])]

                latency_data.append(data)
            
                if j != data[0]:
                    loss = loss + 1
                    j = int(data[0])
                
                j = j + 1
        
        latency_logfile.close()
        
        print(f"Packet Loss : {j / RECORD_NUM}")
        latency_df = pd.DataFrame(latency_data, columns = ["ID", "latency"])
        latency_df = latency_df.sort_values(by='latency', ascending=True)
        latency_df["rate"] = 0

        for i in range(len(latency_df)):
            latency_df.iloc[i, 2] =  (float(i + 1) / len(latency_df)) * 100
    
        #latency_fig.add_trace(go.Scatter(x=latency_df["rate"], y=latency_df["latency"], name=cipher_name, line=dict(color=color[color_idx])))
        #color_idx = color_idx + 1
        latency_fig.add_trace(go.Scatter(x=latency_df["rate"], y=latency_df["latency"], name=cipher_name))

        x_min, x_max = 0, 101
        y_min, y_max = min(math.floor(min(latency_df["latency"])), y_min) , max(math.ceil(max(latency_df["latency"])), y_max)

    if y_min % 8 != 0:
        y_min = (math.floor(y_min / 8)) * 8 - 8
    if y_max % 8 != 0:
        y_max = (math.floor(y_max / 8)) * 8 + 8

    latency_fig.update_layout(
        xaxis = dict(
            title = "Percentage (%)",
            range = [x_min, x_max],
            dtick =10
        ),
        yaxis = dict(
            title = "Latency (ms)",
            range = [y_min, y_max],
            dtick = 8
        ),
        font=dict(
            family = "Times New Roman",
            size=36,
        ),
    )
    latency_fig.update_traces(
        mode="lines",
        #marker=dict(
        #    size=0,
        #),
        line=dict(
            width=2,
        )
    )
    latency_fig.show()

    if DEBUG:
        print(latency_df)


if __name__ == "__main__":

    # TLS 
    #client_dirlist = ['2021-08-26-11_30_14', '2021-08-26-11_34_47', '2021-08-26-11_39_05', '2021-08-26-11_43_59', '2021-08-26-11_47_14']
    #server_dirlist = ['2021-08-26-11:30:07', '2021-08-26-11:34:08', '2021-08-26-11:38:51', '2021-08-26-11:43:38', '2021-08-26-11:47:06']
    #cipher_name_list = ["TLS_AES_128_GCM_SHA256", "TLS_AES_256_GCM_SHA256", "TLS_AES_128_CCM_SHA256", "TLS_AES_128_CCM_8_SHA256", "TLS_CHACHA20_POLY1305_SHA256"]
    #protocol_name = "TLS"

    # DTLS 
    client_dirlist = ['2021-08-26-11_51_52', '2021-08-26-11_55_28', '2021-08-26-11_58_41', '2021-08-26-12_02_17', '2021-08-26-12_05_34', '2021-08-26-18:40:15', '2021-08-26-18:51:19']
    server_dirlist = ['2021-08-26-11:51:32', '2021-08-26-11:55:12', '2021-08-26-11:58:32', '2021-08-26-12:02:08', '2021-08-26-12:05:24', '2021-08-26-18:39:07', '2021-08-26-18:51:13']
    cipher_name_list = ["TLS_AES_128_GCM_SHA256", "TLS_AES_256_GCM_SHA256", "TLS_AES_128_CCM_SHA256", "TLS_AES_128_CCM_8_SHA256", "TLS_CHACHA20_POLY1305_SHA256", "TLS_RSA_WITH_AES_128_CBC_SHA", "TLS_RSA_WITH_AES_256_CBC_SHA"]
    protocol_name = "DTLS"

    # QUIC 
    #client_dirlist = ['2021-08-26-12_09_11', '2021-08-26-12_12_43', '2021-08-26-12_16_15']
    #server_dirlist = ['2021-08-26-12:08:52', '2021-08-26-12:12:35', '2021-08-26-12:16:03']
    #cipher_name_list = ["TLS_AES_128_GCM_SHA256", "TLS_AES_256_GCM_SHA256", "TLS_CHACHA20_POLY1305_SHA256"]
    #protocol_name = "QUIC"
    

    time_logfile_name_list, power_logfile_name_list, top_logfile_name_list, latency_logfile_name_list = [], [], [], []
    for client_outdir, server_outdir, cipher_name in zip(client_dirlist, server_dirlist, cipher_name_list):
        time_logfile_name_list.append(out_basedir + client_outdir + "/time.csv")
        #power_logfile_name_list.append(out_basedir + client_outdir + "/powerstat.log")
        top_logfile_name_list.append(out_basedir + client_outdir + "/top.log")
        latency_logfile_name_list.append(server_out_basedir + server_outdir + "/" + protocol_name)
        
    time_data_list = get_time_log(time_logfile_name_list)
    generate_cpu_usage_graph(top_logfile_name_list, time_data_list, cipher_name_list)
    #generate_power_consumption_graph(power_logfile_name_list, time_data_list, cipher_name_list)
    generate_latency_graph(latency_logfile_name_list, cipher_name_list)



