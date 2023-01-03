# CASE STUDY 1: THRASHING AND LOCALITY
import matplotlib.pyplot as plt 
import numpy as np



plt.show()
if (__name__ == "__main__"):
    
    data1 = np.genfromtxt('../new_profiled_data/profile1.dat',
                     names=["Jiffies","MajorPgFault","MinorPgFault","CPU_Util"],
                     dtype=int,
                     encoding="utf-8",
                     delimiter=' ')
    
    minpf_arr_1 = []
    maxpf_arr_1 = []
    jiffies_arr_1 = []
    accumulated_pf = 0
    accumulated_pf_till_sampling_time = []
    
    # getting data in data structures
    for i in data1:
        if(i[0]!=0):
            accumulated_pf+=i[2] #all min pf till sampling time
            accumulated_pf+=i[1] #all maj pf till sampling time
            jiffies_arr_1.append(i[0])
            minpf_arr_1.append(i[2])
            maxpf_arr_1.append(i[1])
            accumulated_pf_till_sampling_time.append(accumulated_pf)
            
    # sampling time is 50ms. So x-axis data is created
    sampling_time = []
    xcount = 0
    for i in range(len(minpf_arr_1)):
        xcount+=50
        sampling_time.append(xcount)
    
    # Plot Create for case 1
    print(sampling_time[0],accumulated_pf_till_sampling_time[0])
    print(len(sampling_time),len(accumulated_pf_till_sampling_time))
    line_1 = plt.plot(sampling_time,accumulated_pf_till_sampling_time)
    plt.xlabel("Sampling time in ms")
    plt.ylabel("Accumulated Major and Minor PF till Sample time")
    plt.savefig("case_study_1_work_1_2")
    plt.show()
    
    data2 = np.genfromtxt('../new_profiled_data/profile2.dat',
                     names=["Jiffies","MajorPgFault","MinorPgFault","CPU_Util"],
                     dtype=int,
                     encoding="utf-8",
                     delimiter=' ')
    minpf_arr_2 = []
    maxpf_arr_2 = []
    jiffies_arr_2 = []
    accumulated_pf_2 = 0
    accumulated_pf_till_sampling_time_2 = []
    
    # getting data in data structures
    for i in data2:
        if(i[0]!=0):
            accumulated_pf_2+=i[2] #all min pf till sampling time
            accumulated_pf_2+=i[1] #all maj pf till sampling time
            jiffies_arr_2.append(i[0])
            minpf_arr_2.append(i[2])
            maxpf_arr_2.append(i[1])
            accumulated_pf_till_sampling_time_2.append(accumulated_pf_2)
            
    # sampling time is 50ms. So x-axis data is created
    sampling_time_2 = []
    xcount = 0
    for i in range(len(minpf_arr_2)):
        xcount+=50
        sampling_time_2.append(xcount)
        
    # Plot Create
    print(len(sampling_time_2),len(accumulated_pf_till_sampling_time_2))
    plt.plot(sampling_time_2,accumulated_pf_till_sampling_time_2,color="orange")
    
    plt.xlabel("Sampling time in ms")
    plt.ylabel("Accumulated Major and Minor PF till Sample time")
    plt.savefig("case_study_1_work_3_4")
    plt.show()
    
    # Compare in same graph
    plt.plot(sampling_time,accumulated_pf_till_sampling_time,color="red")
    line_2 = plt.plot(sampling_time_2,accumulated_pf_till_sampling_time_2,color="orange")
    plt.xlabel("Sampling time in ms")
    plt.ylabel("Accumulated Major and Minor PF till Sample time")
    plt.legend(("Work 1 & 2","Work 3 & 4"))
    plt.savefig("case_study_1_compare")
    plt.show()
    
    
    
    