###################################################
# Load Inspector stats post processing script
# Author: Rahul Bera (write2bera@gmail.com)
###################################################

import argparse
import matplotlib.pyplot as plt

def add_arguments(parser):
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        default="inspector.stats.txt",
        help="Input stats file name",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default="inspector.stats.png",
        help="Output file name",
    )


def read_file_to_dict(file_path):
    result_dict = {}
    
    with open(file_path, 'r') as file:
        for line in file:
            stripped_line = line.strip()
            if stripped_line:  # Check if the line is not empty
                key, value = stripped_line.split(' ', 1)
                result_dict[key] = (int)(value)
    
    return result_dict

#########################
# MAIN
#########################

parser = argparse.ArgumentParser()
add_arguments(parser)
args = parser.parse_args()

# Read stat
stats = read_file_to_dict(args.input)

# Summarize stats
non_vector_loads = stats["load.non_vector"]
vector_loads = stats["load.vector"]
rip_loads = stats["load.RIP.1B"] + stats["load.RIP.2B"] + stats["load.RIP.4B"] + stats["load.RIP.8B"] + stats["load.RIP.16B"] + stats["load.RIP.32B"] + stats["load.RIP.64B"]
stack_loads = stats["load.STACK.1B"] + stats["load.STACK.2B"] + stats["load.STACK.4B"] + stats["load.STACK.8B"] + stats["load.STACK.16B"] + stats["load.STACK.32B"] + stats["load.STACK.64B"]
reg_loads = stats["load.REG.1B"] + stats["load.REG.2B"] + stats["load.REG.4B"] + stats["load.REG.8B"] + stats["load.REG.16B"] + stats["load.REG.32B"] + stats["load.REG.64B"]
loads_1B = stats["load.RIP.1B"] + stats["load.STACK.1B"] + stats["load.REG.1B"]
loads_2B = stats["load.RIP.2B"] + stats["load.STACK.2B"] + stats["load.REG.2B"]
loads_4B = stats["load.RIP.4B"] + stats["load.STACK.4B"] + stats["load.REG.4B"]
loads_8B = stats["load.RIP.8B"] + stats["load.STACK.8B"] + stats["load.REG.8B"]
loads_16B = stats["load.RIP.16B"] + stats["load.STACK.16B"] + stats["load.REG.16B"]
loads_32B = stats["load.RIP.32B"] + stats["load.STACK.32B"] + stats["load.REG.32B"]
loads_64B = stats["load.RIP.64B"] + stats["load.STACK.64B"] + stats["load.REG.64B"]
global_stable_loads = stats["global_stable_loads.total"]
non_global_stable_loads = non_vector_loads - global_stable_loads # since the tool only checks non-vector loads for stability
global_stable_loads_rip = stats["global_stable_loads.RIP.1B"] + stats["global_stable_loads.RIP.2B"] + stats["global_stable_loads.RIP.4B"] + stats["global_stable_loads.RIP.8B"] + stats["global_stable_loads.RIP.16B"] + stats["global_stable_loads.RIP.32B"] + stats["global_stable_loads.RIP.64B"]
global_stable_loads_stack = stats["global_stable_loads.STACK.1B"] + stats["global_stable_loads.STACK.2B"] + stats["global_stable_loads.STACK.4B"] + stats["global_stable_loads.STACK.8B"] + stats["global_stable_loads.STACK.16B"] + stats["global_stable_loads.STACK.32B"] + stats["global_stable_loads.STACK.64B"]
global_stable_loads_reg = stats["global_stable_loads.REG.1B"] + stats["global_stable_loads.REG.2B"] + stats["global_stable_loads.REG.4B"] + stats["global_stable_loads.REG.8B"] + stats["global_stable_loads.REG.16B"] + stats["global_stable_loads.REG.32B"] + stats["global_stable_loads.REG.64B"]

# Create a figure with six subplots
fig, axs = plt.subplots(1, 5, figsize=(25, 5))
# fig.suptitle('Pie Charts Distribution', fontsize=20, fontweight='bold', fontfamily='serif')

# Common settings
colors = ['#ff9999', '#66b3ff', '#99ff99', '#ffcc99']
explode = (0.02, 0.02, 0.02, 0.02)  # explode all slices

# Define common properties for all pie charts
pie_kwargs = {
    'autopct': '%1.1f%%',
    'startangle': 140,
}

# vector/non-vector pie
axs[0].pie([vector_loads, non_vector_loads], labels=["Vector", "Non-vector"], colors=['#ff9999', '#66b3ff'], explode=[0.02, 0.02], **pie_kwargs)
axs[0].set_title('Vector load fraction', fontsize=14, fontweight='bold', fontfamily='monospace')

# load-size wise pie
axs[1].pie([loads_1B, loads_2B, loads_4B, loads_8B, loads_16B, loads_32B, loads_64B], labels=["1B", "2B", "4B", "8B", "16B", "32B", "64B"], colors=['#ff9999', '#66b3ff', '#99ff99', '#ffcc99', '#c2c2f0', '#ffb3e6', '#c4e17f'], explode=[0.02, 0.02, 0.02, 0.02, 0.02, 0.02, 0.02], **pie_kwargs)
axs[1].set_title('Loads by size', fontsize=14, fontweight='bold', fontfamily='monospace')

# addressing-mode wise pie
axs[2].pie([rip_loads, stack_loads, reg_loads], labels=["PC-rel", "Stack-rel", "Reg-rel"], colors=['#ff9999', '#66b3ff', '#99ff99'], explode=[0.02, 0.02, 0.02], **pie_kwargs)
axs[2].set_title('Loads by addressing mode', fontsize=14, fontweight='bold', fontfamily='monospace')

# Global-stable-load pie
axs[3].pie([global_stable_loads, non_global_stable_loads], labels=["Global-stable", "Non-global-stable"], colors=['#ff9999', '#66b3ff'], explode=[0.02, 0.02], **pie_kwargs)
axs[3].set_title('Global-stable loads', fontsize=14, fontweight='bold', fontfamily='monospace')

# Global-stable loads by addressing mode
axs[4].pie([global_stable_loads_rip, global_stable_loads_stack, global_stable_loads_reg], labels=["PC-rel", "Stack-rel", "Reg-rel"], colors=['#ff9999', '#66b3ff', '#99ff99'], explode=[0.02, 0.02, 0.02], **pie_kwargs)
axs[4].set_title('Global-stable loads by addressing mode', fontsize=14, fontweight='bold', fontfamily='monospace')

# Adjust layout to prevent overlap
plt.tight_layout(rect=[0, 0.03, 1, 0.95])

# Save the figure as a PNG file
plt.savefig(args.output, dpi=300)

# Show the plot
plt.show()