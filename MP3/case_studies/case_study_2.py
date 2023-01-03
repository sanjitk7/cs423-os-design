# CASE STUDY 2: CPU UTILIZATION
import matplotlib.pyplot as plt 
import numpy as np

def get_data(file):
    return np.genfromtxt(file,
                    names=["Jiffies","MajorPgFault","MinorPgFault","CPU_Util"],
                    dtype=int,
                    encoding="utf-8",
                    delimiter=' ')

def get_cpu_util(data):
    cpu_time_sum = 0
    cpu_util_sum = 0
    first_jiffy = data[0][0]
    completion_time = 1
    print("first_jiffy: ",first_jiffy)
    for i in data:
        if (i[0]>0):
            cpu_time = i[3]
            cpu_time_sum +=cpu_time
            curr_jiffy = i[0]
            # print("curr_jiffy: ",curr_jiffy)
            if (curr_jiffy!=first_jiffy):
                completion_time = curr_jiffy - first_jiffy
                cpu_util = cpu_time/completion_time
                cpu_util_sum+=cpu_util
    return cpu_util_sum
            
            

data1 = get_data("../new_profiled_data/profile3_1.dat")
data5 = get_data("../new_profiled_data/profile3_5.dat")
data11 = get_data("../new_profiled_data/profile3_11.dat")
data16 = get_data("../new_profiled_data/profile3_16.dat")
# data20 = get_data("../new_profiled_data/profile3_20.dat")
# data22 = get_data("../new_profiled_data/profile3_22.dat")
# data24 = get_data("../new_profiled_data/profile3_24.dat")

x = [1,5,11,16]
y = []

y.append(get_cpu_util(data1))
y.append(get_cpu_util(data5))
y.append(get_cpu_util(data11))
y.append(get_cpu_util(data16))
# y.append(get_cpu_util(data20))
# y.append(get_cpu_util(data24))

plt.plot(x,y,color="orange")
plt.xlabel("Degree of Multiprogramming")
plt.ylabel("CPU UTIL")
plt.savefig("case_study_2")
plt.show()

