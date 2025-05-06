def plot_index_size_comparison():
    import matplotlib.pyplot as plt
    import pandas as pd

    # Load all result CSVs
    base_path = 'results/fb_100M_public_uint64_ops_2M_0.000000rq_0.500000nl_'
    workloads = {
        "Lookup Only": f"{base_path}0.000000i_results_table.csv",
        "50% Insert": f"{base_path}0.500000i_0m_results_table.csv",
        "10% Insert Mix": f"{base_path}0.100000i_0m_mix_results_table.csv",
        "90% Insert Mix": f"{base_path}0.900000i_0m_mix_results_table.csv",
    }

    index_names = ["DynamicPGM", "LIPP", "HybridPGMLIPP", "HybridDoubleBuffer"]

    size_data = {index: [] for index in index_names}

    for label, path in workloads.items():
        df = pd.read_csv(path)
        for idx in index_names:
            row = df[df["index_name"] == idx]
            if not row.empty:
                size_data[idx].append(row["index_size_bytes"].values[0])
            else:
                size_data[idx].append(0)

    # Plotting
    fig, ax = plt.subplots(figsize=(8, 6))
    bar_width = 0.25
    index = range(len(workloads))
    colors = ['#a6cee3', '#b2df8a', '#fb9a99', '#fdbf6f']

    for i, idx in enumerate(index_names):
        ax.bar([x + i*bar_width for x in index], size_data[idx], bar_width, label=idx, color=colors[i])

    ax.set_title('Index Size Comparison Across Workloads')
    ax.set_ylabel('Index Size (Bytes)')
    ax.set_xticks([x + bar_width for x in index])
    ax.set_xticklabels(workloads.keys(), rotation=15)
    ax.legend()
    ax.grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()
    plt.savefig('index_size_comparison.png', dpi=300)
    plt.show()

# At the bottom of analysis.py

if __name__ == "__main__":
    # result_analysis()  # <- This must be defined above
    plot_index_size_comparison()  # <- This must also be defined above

