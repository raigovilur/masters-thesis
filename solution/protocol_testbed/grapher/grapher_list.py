#!/usr/bin/env python3

import argparse, re, sys, math
import pandas as pd
from datetime import time, timedelta
import plotly.express as px
import plotly.graph_objects as go

DEBUG = True

out_basedir = "../out/"


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


def generate_cpu_usage_graph(top_logfile_name_list, time_data_list, protocol_name_list):
    
    top_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0

    for top_logfile_name, time_data, protocol_name in zip(top_logfile_name_list, time_data_list, protocol_name_list):
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
            gt = time_data["File sent"]

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
       
        top_fig.add_trace(go.Scatter(x=top_df["Rate"], y = top_df["%CPU"], name=protocol_name))

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
            size=8,
        ),
        line=dict(
            width=4,
        )
    )
    top_fig.show()

def generate_power_consumption_graph(power_logfile_name_list, time_data_list, protocol_name_list):
    
    power_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    for power_logfile_name, time_data, protocol_name in zip(power_logfile_name_list, time_data_list, protocol_name_list):
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
        
        power_fig.add_trace(go.Scatter(x=power_df["Rate"], y=power_df["Watts"], name=protocol_name))
    
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
            size=8,
        ),
        line=dict(
            width=4,
        )
    )
    power_fig.show()

    if DEBUG:
        print(power_df)

'''
def generate_throughput_graph(throughput_logfile_name_list, protocol_name_list):

    throughput_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    for power_logfile_name, protocol_name in zip(throughput_logfile_name_list, protocol_name_list):
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
        
        x_min, x_max = 0, 100
        y_min, y_max = min(math.floor(min(throughput_df["throughput"])), y_min) , max(math.ceil(max(throughput_df["throughput"])), y_max)
    
    if y_min % 2 != 0:
        y_min = y_min - 1
    if y_max % 2 != 0:
        y_max = y_max + 1

    throughput_fig.update_layout(
        xaxis = dict(
            title = "Percentage of received file (%)",
            range = [x_min, x_max],
            dtick = 10
        ),
        yaxis = dict(
            title = "Throughput (Mb/s)",
            range = [y_min, y_max],
            dtick = 2
        ),
        font=dict(
            family = "Times New Roman",
            size=36,
        ),
    )
    throughput_fig.update_traces(
        mode="lines+markers",
        marker=dict(
            size=10,
        ),
        line=dict(
            width=4,
        )
    )
    throughput_fig.show()

    if DEBUG:
        print(throughput_df)
'''

def generate_throughput_sorted_graph(throughput_logfile_name_list, protocol_name_list):
    
    throughput_fig = go.Figure()
    x_min, x_max = 1000000, 0
    y_min, y_max = 1000000, 0
    for throughput_logfile_name, protocol_name in zip(throughput_logfile_name_list, protocol_name_list):
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
    
        throughput_fig.add_trace(go.Scatter(x=throughput_df["rate"], y=throughput_df["throughput"], name=protocol_name))

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
            size=8,
        ),
        line=dict(
            width=4,
        )
    )
    throughput_fig.show()

    if DEBUG:
        print(throughput_df)


if __name__ == "__main__":
    client_dirlist = ["2021-08-22-18:20:11", "2021-08-22-18:27:05", "2021-08-22-18:33:23"]
    server_dirlist = ["2021-08-22-18:23:45", "2021-08-22-18:30:38", "2021-08-22-18:36:40"]
    protocol_name_list = ["TLS", "DTLS", "QUIC"]

    time_logfile_name_list, power_logfile_name_list, top_logfile_name_list, throughput_logfile_name_list = [], [], [], []
    for client_outdir, server_outdir, protocol_name in zip(client_dirlist, server_dirlist, protocol_name_list):
        time_logfile_name_list.append(out_basedir + client_outdir + "/time.csv")
        power_logfile_name_list.append(out_basedir + client_outdir + "/powerstat.log")
        top_logfile_name_list.append(out_basedir + client_outdir + "/top.log")
        throughput_logfile_name_list.append(out_basedir + "server/" + server_outdir + "/" + protocol_name)
        
    time_data_list = get_time_log(time_logfile_name_list)
    generate_cpu_usage_graph(top_logfile_name_list, time_data_list, protocol_name_list)
    generate_power_consumption_graph(power_logfile_name_list, time_data_list, protocol_name_list)
    #generate_throughput_graph(throughput_logfile_name_list, protocol_name_list)
    generate_throughput_sorted_graph(throughput_logfile_name_list, protocol_name_list)


