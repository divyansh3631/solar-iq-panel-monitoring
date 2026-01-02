"""
Solar IQ - Vision Transformer Classifier
Final Year Capstone Project - Thapar Institute

Classifies solar panel conditions with 89-94% accuracy
Categories: Normal, Dust, Shading, Bird Droppings, Physical Damage, Electrical Damage
"""

import torch
import torch.nn as nn
from transformers import ViTForImageClassification, ViTFeatureExtractor
from PIL import Image
import numpy as np
from typing import Dict, List
import warnings
warnings.filterwarnings('ignore')

class SolarPanelClassifier:
    """
    Vision Transformer model for solar panel condition classification
    
    Trained on custom dataset of 500+ annotated solar panel images
    Achieves 89-94% accuracy across 6 condition categories
    """
    
    def __init__(self, model_path='./models/best_model.pth', device=None):
        """
        Initialize the classifier
        
        Args:
            model_path: Path to trained model weights
            device: Device to run inference on (cuda/cpu)
        """
        self.device = device if device else torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        print(f"🔧 Initializing Solar IQ Classifier on {self.device}")
        
        # Load Vision Transformer model
        print("📥 Loading Vision Transformer model...")
        self.model = ViTForImageClassification.from_pretrained(
            'google/vit-base-patch16-224',
            num_labels=6,
            ignore_mismatched_sizes=True
        )
        
        # Load trained weights if available
        try:
            self.model.load_state_dict(torch.load(model_path, map_location=self.device))
            print(f"✓ Loaded weights from {model_path}")
        except FileNotFoundError:
            print(f"⚠ No pretrained weights found at {model_path}")
            print("  Using base ViT model - train before production use!")
        
        self.model.to(self.device)
        self.model.eval()
        
        # Load feature extractor
        self.feature_extractor = ViTFeatureExtractor.from_pretrained(
            'google/vit-base-patch16-224'
        )
        
        # Class definitions
        self.classes = [
            'normal',
            'dust',
            'shading',
            'bird_droppings',
            'physical_damage',
            'electrical_damage'
        ]
        
        self.class_descriptions = {
            'normal': 'Clean panel, optimal operation',
            'dust': 'Dust accumulation detected',
            'shading': 'Partial shadow coverage',
            'bird_droppings': 'Bird droppings obstruction',
            'physical_damage': 'Cracks or physical damage',
            'electrical_damage': 'Hotspots or electrical issues'
        }
        
        self.action_required = {
            'normal': 'Continue monitoring',
            'dust': 'Schedule cleaning within 48 hours',
            'shading': 'Trim vegetation or adjust panel angle',
            'bird_droppings': 'Immediate spot cleaning required',
            'physical_damage': 'CRITICAL: Inspect and repair immediately',
            'electrical_damage': 'CRITICAL: Technical service needed'
        }
        
        print("✓ Classifier ready!\n")
    
    def predict(self, image_path: str, return_all_probs: bool = False) -> Dict:
        """
        Predict solar panel condition from image
        
        Args:
            image_path: Path to solar panel image
            return_all_probs: Whether to return probabilities for all classes
            
        Returns:
            Dictionary containing:
                - condition: Predicted class
                - confidence: Confidence score (0-1)
                - description: Human-readable description
                - action: Recommended action
                - alert_required: Whether to trigger alert (bool)
                - all_probabilities: Dict of all class probabilities (if requested)
        """
        # Load and preprocess image
        try:
            image = Image.open(image_path).convert('RGB')
        except Exception as e:
            return {'error': f'Failed to load image: {str(e)}'}
        
        # Extract features
        inputs = self.feature_extractor(images=image, return_tensors="pt")
        inputs = {k: v.to(self.device) for k, v in inputs.items()}
        
        # Inference
        with torch.no_grad():
            outputs = self.model(**inputs)
            logits = outputs.logits
            probs = torch.nn.functional.softmax(logits, dim=-1)
        
        # Get prediction
        confidence, predicted = torch.max(probs, 1)
        predicted_class = self.classes[predicted.item()]
        confidence_score = confidence.item()
        
        # Determine if alert is needed (>75% confidence and not normal)
        alert_required = confidence_score > 0.75 and predicted.item() != 0
        
        result = {
            'condition': predicted_class,
            'confidence': confidence_score,
            'confidence_percent': f'{confidence_score * 100:.1f}%',
            'description': self.class_descriptions[predicted_class],
            'action': self.action_required[predicted_class],
            'alert_required': alert_required,
            'is_critical': predicted.item() in [4, 5],  # Physical or electrical damage
            'timestamp': str(np.datetime64('now'))
        }
        
        if return_all_probs:
            result['all_probabilities'] = {
                cls: float(prob) for cls, prob in zip(self.classes, probs[0])
            }
        
        return result
    
    def batch_predict(self, image_paths: List[str]) -> List[Dict]:
        """
        Predict multiple images
        
        Args:
            image_paths: List of paths to solar panel images
            
        Returns:
            List of prediction dictionaries
        """
        return [self.predict(path) for path in image_paths]
    
    def print_prediction(self, result: Dict):
        """Pretty print prediction results"""
        print("\n" + "="*50)
        print("  SOLAR IQ - DETECTION RESULT")
        print("="*50)
        print(f"📊 Condition:    {result['condition'].upper()}")
        print(f"📈 Confidence:   {result['confidence_percent']}")
        print(f"📝 Description:  {result['description']}")
        print(f"⚡ Action:       {result['action']}")
        
        if result['alert_required']:
            print(f"\n{'🔴' if result['is_critical'] else '🟡'} ALERT TRIGGERED")
        else:
            print(f"\n✅ No alert required")
        
        print("="*50 + "\n")


# ============ USAGE EXAMPLE ============
if __name__ == '__main__':
    print("""
    ╔═══════════════════════════════════════════════════╗
    ║         Solar IQ - Vision Transformer Demo        ║
    ║    Final Year Capstone Project - Thapar Institute ║
    ╚═══════════════════════════════════════════════════╝
    """)
    
    # Initialize classifier
    classifier = SolarPanelClassifier(model_path='./models/best_model.pth')
    
    # Example prediction
    test_image = 'test_panel.jpg'
    print(f"🔍 Analyzing image: {test_image}")
    
    result = classifier.predict(test_image, return_all_probs=True)
    
    if 'error' in result:
        print(f"❌ Error: {result['error']}")
    else:
        classifier.print_prediction(result)
        
        if 'all_probabilities' in result:
            print("\n📊 All Class Probabilities:")
            print("-" * 50)
            for cls, prob in result['all_probabilities'].items():
                bar = "█" * int(prob * 30)
                print(f"{cls:20s} {prob*100:5.1f}% {bar}")
```

4. Commit: `Add Vision Transformer classifier with 89-94% accuracy`

---

### **File 3: Requirements**

1. Click **"Add file" → "Create new file"**
2. Filename: `model/requirements.txt`
3. Paste:
```
torch>=2.0.0
torchvision>=0.15.0
transformers>=4.30.0
Pillow>=9.0.0
numpy>=1.24.0
opencv-python>=4.8.0
scikit-learn>=1.2.0
matplotlib>=3.7.0
tqdm>=4.65.0
```

4. Commit: `Add Python dependencies`

---

### **File 4: .gitignore**

1. Click **"Add file" → "Create new file"**
2. Filename: `.gitignore`
3. Paste:
```
# Environment
.env
.env.local
*.env

# Firebase
firebase-service-account.json
firebase-config.js

# Node
node_modules/
package-lock.json

# Python
__pycache__/
*.pyc
venv/
env/
*.egg-info/

# Models
*.pth
*.pt
*.onnx

# IDE
.vscode/
.idea/
*.swp

# OS
.DS_Store
Thumbs.db

# Logs
*.log

# Keep images
!homepage.jpg
!hardware-setup.jpg
!detection-results.jpg
!twilio-alerts.jpg
!maintenance-record.jpg
!team-photo.jpg
