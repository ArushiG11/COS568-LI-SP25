import pandas as pd
import matplotlib.pyplot as plt
import os

def result_analysis_with_index_size():
    tasks = ['fb', 'osmc', 'books']
    indexs = ["DynamicPGM", "LIPP", "HybridPGMLIPP", "HybridDoubleBuffer"]
    index_names = ["Dynamic PGM", "LIPP", "Hybrid", "Doubled Buffer"]

    # Store only mixed workload throughput and index sizes
    insertlookup_mix1_throughput = {}
    insertlookup_mix2_throughput = {}
    index_sizes = {}

    for index in indexs:
        insertlookup_mix1_throughput[index] = {}
        insertlookup_mix2_throughput[index] = {}
        index_sizes[index] = {}

    for task in tasks:
        full_task_name = f"{task}_100M_public_uint64"
        try:
            mix_10 = pd.read_csv(f"results/{full_task_name}_ops_2M_0.000000rq_0.500000nl_0.100000i_0m_mix_results_table.csv")
            mix_90 = pd.read_csv(f"results/{full_task_name}_ops_2M_0.000000rq_0.500000nl_0.900000i_0m_mix_results_table.csv")
        except FileNotFoundError:
            continue

        for index in indexs:
            try:
                mix_10_result = mix_10[mix_10['index_name'] == index]
                mix_90_result = mix_90[mix_90['index_name'] == index]

                insertlookup_mix1_throughput[index][task] = mix_10_result[
                    ['mixed_throughput_mops1', 'mixed_throughput_mops2', 'mixed_throughput_mops3']
                ].mean(axis=1).max()

                insertlookup_mix2_throughput[index][task] = mix_90_result[
                    ['mixed_throughput_mops1', 'mixed_throughput_mops2', 'mixed_throughput_mops3']
                ].mean(axis=1).max()

                index_sizes[index][task] = mix_90_result['index_size_bytes'].max()
            except:
                pass

    # Plot throughput for both mixed workloads
    fig, axs = plt.subplots(1, 2, figsize=(14, 6))
    bar_width = 0.2
    index = range(len(indexs))
    colors = ['blue', 'green', 'red']

    for i, task in enumerate(tasks):
        task_data = [insertlookup_mix1_throughput[idx].get(task, 0) for idx in indexs]
        axs[0].bar([x + i * bar_width for x in index], task_data, bar_width, label=task, color=colors[i])
    axs[0].set_title('Mixed Workload Throughput (10% insert)')
    axs[0].set_ylabel('Throughput (Mops/s)')
    axs[0].set_xticks([x + bar_width for x in index])
    axs[0].set_xticklabels(indexs)
    axs[0].legend()

    for i, task in enumerate(tasks):
        task_data = [insertlookup_mix2_throughput[idx].get(task, 0) for idx in indexs]
        axs[1].bar([x + i * bar_width for x in index], task_data, bar_width, label=task, color=colors[i])
    axs[1].set_title('Mixed Workload Throughput (90% insert)')
    axs[1].set_ylabel('Throughput (Mops/s)')
    axs[1].set_xticks([x + bar_width for x in index])
    axs[1].set_xticklabels(indexs)
    axs[1].legend()

    plt.tight_layout()
    plt.savefig('mixed_throughput_comparison_final.png')
    plt.show()

    # Plot index size comparison
    fig, ax = plt.subplots(figsize=(10, 6))
    for i, task in enumerate(tasks):
        task_data = [index_sizes[idx].get(task, 0) / (1024 * 1024) for idx in indexs]  # convert to MB
        ax.bar([x + i * bar_width for x in index], task_data, bar_width, label=task, color=colors[i])
    ax.set_title('Index Size Comparison (Mixed Workload)')
    ax.set_ylabel('Index Size (MB)')
    ax.set_xticks([x + bar_width for x in index])
    ax.set_xticklabels(indexs)
    ax.legend()

    plt.tight_layout()
    plt.savefig('index_size_comparison_final.png')
    plt.show()

    # Save CSVs
    os.makedirs('analysis_results', exist_ok=True)
    pd.DataFrame(insertlookup_mix1_throughput).to_csv('analysis_results/insertlookup_mix1_throughput.csv')
    pd.DataFrame(insertlookup_mix2_throughput).to_csv('analysis_results/insertlookup_mix2_throughput.csv')
    pd.DataFrame(index_sizes).to_csv('analysis_results/index_sizes.csv')

result_analysis_with_index_size()
