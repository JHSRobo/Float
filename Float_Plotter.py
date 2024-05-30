import serial
import re
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import scienceplots
import tkinter as tk

def get_profile_number(start_str: str) -> int:
    pattern = r'---start of profile (\d+) data---'
    # Use re.search to find the pattern in the text
    match = re.search(pattern, start_str)

    if match:
        # Extract the number from the match object
        return match.group(1)
    else:
        return 0
    
############################# main program ######################################
com = 'COM5'
baud = 9600

serial_inst = serial.Serial(com, baud, timeout=1)

profiling_done = False
is_read_profile_data = False
profile_count = 1

print()
while serial_inst.isOpen() is True:
    data = str(serial_inst.readline().decode("utf-8").rstrip())
    if data != '':
        print(data)     
        if "---start of profile" in data:
            # extract the profile number
            profile_count = get_profile_number(data)             
            is_read_profile_data = True
            data_file = open(f"float_profile{profile_count}.csv", mode='w')
            data_file.truncate()
            data_file.write(f"Time Depth Pressure\n")                

        if (is_read_profile_data and "EX15" in data):
            tokens = data.split(' ')
            data_file.write(f"{tokens[1]} {tokens[3]} {tokens[5]}\n")
            data_file.flush()

        if "---end---" in data:
            is_read_profile_data = False
            data_file.close()
            print(f"generating graph for {profile_count}")              
            plt.style.use(["science", "grid", "notebook"])
            plt.rcParams["font.family"] = "Georgia"
            fig, axes = plt.subplots(1, 1, figsize=(10,5))
            profile_data = pd.read_csv(f"float_profile{profile_count}.csv", sep=' ')
            x = profile_data.loc[:, "Time"].tolist() #get x=time from dataframe
            y = profile_data.loc[:, "Depth"].tolist()
            axes.plot(x, y, "o--", color="blue", lw=0.7, ms=5.0) #marker is o, line is dash, ls=line width, ms=marker size
            axes.set_title(f"P.O.D - Profile {profile_count}", fontsize=20, fontweight="bold", color="black")
            axes.set_xlabel("Time Elapsed (seconds)")
            axes.set_ylabel("Depth (meters)")
            fig_mgr = plt.get_current_fig_manager()
            fig_mgr.set_window_title("Jesuit High School Robotics")
            jesuit_icon = tk.PhotoImage(file="jesuit-logo.gif")
            fig_mgr.window.iconphoto(False, jesuit_icon)
            plt.show()
            