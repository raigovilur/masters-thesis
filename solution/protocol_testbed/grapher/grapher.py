#!/usr/bin/env python3

import argparse, re, sys, math
import pandas as pd
from datetime import time, timedelta
import plotly.express as px
import plotly.graph_objects as go

DEBUG = True

out_basedir = "../out/"

def get_time_log():
    time_logfile = open(time_logfile_name, "r")
    time_data = {}
    for line in time_logfile:
        data = line.rstrip().split(",")
        time_data[data[1]] = time.fromisoformat(data[0])

    time_logfile.close()
    return time_data


def generate_cpu_usage_graph(top_logfile_name, time_data):
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
    top_fig = px.line(top_df, x="TIME", y="%CPU")

    [x_min, x_max] = [0, max(top_df["TIME"])]
    [y_min, y_max] = [0, 100]

    top_fig.update_layout(
        xaxis = dict(
            title = "Excecution time (s)",
            range = [x_min, x_max],
            dtick = 100
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
        mode="lines",
        line=dict(
            width=2,
        )
    )
    top_fig.show()

    if DEBUG:
        print(top_df)

def generate_power_consumption_graph(power_logfile_name, time_data):
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
    power_fig = px.line(power_df, x="Time", y="Watts")
    
    [x_min, x_max] = [0, max(power_df["Time"])]
    [y_min, y_max] = [min(power_df["Watts"]), max(power_df["Watts"])]
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
            title = "Excecution time (s)",
            range = [x_min, x_max],
            dtick = 100
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
        mode="lines",
        line=dict(
            width=2,
        )
    )
    power_fig.show()

    if DEBUG:
        print(power_df)


def generate_throughput_graph(throughput_logfile_name):
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
    throughput_fig = px.line(throughput_df, x="transferred", y="throughput")

    [x_min, x_max] = [0, 100]
    [y_min, y_max] = [math.floor(min(throughput_df["throughput"])), math.ceil(max(throughput_df["throughput"]))]
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

def generate_throughput_sorted_graph(throughput_logfile_name):
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
    
    throughput_fig = px.line(throughput_df, x="rate", y="throughput")

    [x_min, x_max] = [0, 100]
    [y_min, y_max] = [math.floor(min(throughput_df["throughput"])), math.ceil(max(throughput_df["throughput"]))]
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Grapher for protocol tested')
    parser.add_argument('--clientdir', type=str, default="current", help='Directory that have logfile generated by server')
    parser.add_argument('--serverdir', type=str, default="server/current", help='Directory that have logfile generated by client')
    parser.add_argument('protocol', type=str, help='Directory that have logfile generated by client')
    
    args = parser.parse_args()
    client_outdir = args.clientdir
    server_outdir = args.serverdir
    protocol_name = args.protocol

    time_logfile_name = out_basedir + client_outdir + "/time.csv"
    power_logfile_name = out_basedir + client_outdir + "/powerstat.log"
    top_logfile_name = out_basedir + client_outdir + "/top.log"
    throughput_logfile_name = out_basedir + server_outdir + "/" + protocol_name
    
    time_data = get_time_log()
    generate_cpu_usage_graph(top_logfile_name, time_data)
    generate_power_consumption_graph(power_logfile_name, time_data)
    generate_throughput_graph(throughput_logfile_name)
    generate_throughput_sorted_graph(throughput_logfile_name)



