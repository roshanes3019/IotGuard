import matplotlib.pyplot as plt
import networkx as nx
import os

# Set up the directory to save the graphs
output_dir = "graphs"
os.makedirs(output_dir, exist_ok=True)

# 1. Network Communication Diagram
G = nx.DiGraph()
G.add_edges_from([
    ("Trusted Device 1", "ESP8266 Firewall"),
    ("Trusted Device 2", "ESP8266 Firewall"),
    ("ESP8266 Firewall", "Trusted Device 1"),
    ("ESP8266 Firewall", "Trusted Device 2"),
])
pos = nx.spring_layout(G)
nx.draw(G, pos, with_labels=True, node_size=3000, node_color='lightblue', font_size=10)
plt.title("Network Communication Diagram")
plt.savefig(os.path.join(output_dir, "network_communication_diagram.png"))
plt.clf()

# 2. Data Flow Diagram
H = nx.DiGraph()
H.add_edges_from([
    ("Input Device", "ESP-NOW Layer"),
    ("ESP-NOW Layer", "Firewall Security Module"),
    ("Firewall Security Module", "Output Device"),
    ("Firewall Security Module", "Data Storage"),
])
pos = nx.spring_layout(H)
nx.draw(H, pos, with_labels=True, node_size=3000, node_color='lightgreen', font_size=10)
plt.title("Data Flow Diagram")
plt.savefig(os.path.join(output_dir, "data_flow_diagram.png"))
plt.clf()

# 3. Encryption Process Flow
I = nx.DiGraph()
I.add_edges_from([
    ("Plain Data", "Encryption Algorithm"),
    ("Encryption Algorithm", "Encrypted Data"),
    ("Encrypted Data", "ESP8266 Firewall"),
])
pos = nx.spring_layout(I)
nx.draw(I, pos, with_labels=True, node_size=3000, node_color='lightcoral', font_size=10)
plt.title("Encryption Process Flow")
plt.savefig(os.path.join(output_dir, "encryption_process_flow.png"))
plt.clf()

# 4. Chunk Transmission Visualization
chunks = [1, 2, 3, 4, 5]
success_rates = [95, 93, 90, 88, 85]  # Hypothetical data
plt.plot(chunks, success_rates, marker='o', color='purple')
plt.title("Chunk Transmission Success Rate")
plt.xlabel("Chunk ID")
plt.ylabel("Success Rate (%)")
plt.grid(True)
plt.savefig(os.path.join(output_dir, "chunk_transmission_success.png"))
plt.clf()

# 5. Device Verification Timeline
time_points = [1, 2, 3, 4, 5]
verification_times = [0.5, 0.7, 0.65, 0.6, 0.55]  # Hypothetical data
plt.plot(time_points, verification_times, marker='o', color='orange')
plt.title("Device Verification Timeline")
plt.xlabel("Time Point")
plt.ylabel("Verification Time (seconds)")
plt.grid(True)
plt.savefig(os.path.join(output_dir, "device_verification_timeline.png"))
plt.clf()

print(f"Graphs saved in the '{output_dir}' directory.")
