#!/usr/bin/env python3
import random
import sys

def generate_sprint_health_data(num_patients=1000, num_doctors=50):
    """Generate test data for Sprint Health case study"""
    
    triples = []
    
    # Define predicates
    pred_type = "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"
    pred_hasName = "<http://example.org/sprint_health#hasName>"
    pred_hasAppointment = "<http://example.org/sprint_health#hasAppointment>"
    pred_attendedBy = "<http://example.org/sprint_health#attendedBy>"
    pred_hasDate = "<http://example.org/sprint_health#hasDate>"
    pred_hasMeasurement = "<http://example.org/sprint_health#hasMeasurement>"
    pred_hasValue = "<http://example.org/sprint_health#hasValue>"
    
    # Define classes
    class_Person = "<http://example.org/sprint_health#Person>"
    class_Patient = "<http://example.org/sprint_health#Patient>"
    class_Doctor = "<http://example.org/sprint_health#Doctor>"
    class_Appointment = "<http://example.org/sprint_health#Appointment>"
    class_BloodPressure = "<http://example.org/sprint_health#BloodPressure>"
    
    # Generate doctors
    for i in range(num_doctors):
        doctor_id = f"<http://example.org/doctor_{i}>"
        triples.append(f"{doctor_id} {pred_type} {class_Doctor} .")
        triples.append(f'{doctor_id} {pred_hasName} "Dr. Smith {i}" .')
    
    # Generate patients with appointments
    appointment_id = 0
    measurement_id = 0
    
    for i in range(num_patients):
        patient_id = f"<http://example.org/patient_{i}>"
        
        # Patient type and name
        triples.append(f"{patient_id} {pred_type} {class_Patient} .")
        triples.append(f'{patient_id} {pred_hasName} "Patient {i}" .')
        
        # Generate appointments (1-3 per patient)
        num_appointments = random.randint(1, 3)
        for j in range(num_appointments):
            appt_id = f"<http://example.org/appointment_{appointment_id}>"
            appointment_id += 1
            
            # Appointment details
            triples.append(f"{appt_id} {pred_type} {class_Appointment} .")
            triples.append(f"{patient_id} {pred_hasAppointment} {appt_id} .")
            
            # Assign random doctor
            doctor_id = f"<http://example.org/doctor_{random.randint(0, num_doctors-1)}>"
            triples.append(f"{appt_id} {pred_attendedBy} {doctor_id} .")
            triples.append(f'{appt_id} {pred_hasDate} "2024-{random.randint(1,12):02d}-{random.randint(1,28):02d}" .')
        
        # Generate blood pressure measurements (0-2 per patient)
        num_measurements = random.randint(0, 2)
        for j in range(num_measurements):
            measure_id = f"<http://example.org/measurement_{measurement_id}>"
            measurement_id += 1
            
            # Measurement details
            triples.append(f"{measure_id} {pred_type} {class_BloodPressure} .")
            triples.append(f"{patient_id} {pred_hasMeasurement} {measure_id} .")
            
            # Blood pressure value (some high, some normal)
            bp_value = random.randint(110, 180)
            triples.append(f'{measure_id} {pred_hasValue} "{bp_value}" .')
    
    return triples

if __name__ == "__main__":
    if len(sys.argv) > 1:
        num_patients = int(sys.argv[1])
    else:
        num_patients = 1000
    
    triples = generate_sprint_health_data(num_patients)
    
    # Write to file
    with open("/tmp/sprint_health_data.ttl", "w") as f:
        f.write("@prefix : <http://example.org/sprint_health#> .\n")
        f.write("@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n\n")
        
        for triple in triples:
            f.write(triple + "\n")
    
    print(f"Generated {len(triples)} triples for {num_patients} patients")
    print("Data written to: /tmp/sprint_health_data.ttl")
