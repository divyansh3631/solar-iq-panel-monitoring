"""
Solar IQ - Vision Transformer Classifier
Detects solar panel conditions from images
89-94% accuracy across 6 categories
"""

import torch
from transformers import ViTForImageClassification, ViTFeatureExtractor
from PIL import Image

class SolarPanelClassifier:
    """Vision Transformer model for solar panel condition classification"""
    
    def __init__(self, model_path='./best_model.pth'):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        
        # Load model
        self.model = ViTForImageClassification.from_pretrained(
            'google/vit-base-patch16-224',
            num_labels=6
        )
        self.model.load_state_dict(torch.load(model_path, map_location=self.device))
        self.model.to(self.device)
        self.model.eval()
        
        # Load feature extractor
        self.feature_extractor = ViTFeatureExtractor.from_pretrained(
            'google/vit-base-patch16-224'
        )
        
        self.classes = [
            'normal', 'dust', 'shading', 
            'bird_droppings', 'physical_damage', 'electrical_damage'
        ]
    
    def predict(self, image_path):
        """
        Predict solar panel condition from image
        
        Args:
            image_path: Path to panel image
            
        Returns:
            dict with condition, confidence, and all probabilities
        """
        # Load and preprocess image
        image = Image.open(image_path).convert('RGB')
        inputs = self.feature_extractor(images=image, return_tensors="pt")
        inputs = {k: v.to(self.device) for k, v in inputs.items()}
        
        # Inference
        with torch.no_grad():
            outputs = self.model(**inputs)
            logits = outputs.logits
            probs = torch.nn.functional.softmax(logits, dim=-1)
        
        # Get prediction
        confidence, predicted = torch.max(probs, 1)
        
        return {
            'condition': self.classes[predicted.item()],
            'confidence': confidence.item(),
            'all_probabilities': {
                cls: prob.item() 
                for cls, prob in zip(self.classes, probs[0])
            },
            'alert_required': confidence.item() > 0.75 and predicted.item() != 0
        }
    
    def batch_predict(self, image_paths):
        """Predict multiple images"""
        return [self.predict(path) for path in image_paths]

# Usage Example
if __name__ == '__main__':
    classifier = SolarPanelClassifier()
    result = classifier.predict('test_panel.jpg')
    
    print(f"Condition: {result['condition']}")
    print(f"Confidence: {result['confidence']:.2%}")
    print(f"Alert Required: {result['alert_required']}")
```

4. Commit: `Add Vision Transformer classifier for panel detection`

---

### **STEP 4: Create .gitignore File**

1. Click **"Add file"** → **"Create new file"**
2. Name: `.gitignore`
3. Paste:
```
# Environment variables
.env
.env.local
*.env

# Firebase credentials
firebase-service-account.json
firebase-config.js

# Node modules
node_modules/
package-lock.json

# Python
__pycache__/
*.pyc
*.pyo
venv/
env/
*.egg-info/

# Model files
*.pth
*.pt
*.onnx
*.h5

# IDE
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db
desktop.ini

# Logs
*.log
npm-debug.log*

# Build outputs
build/
dist/
*.min.js
*.min.css

# Dataset (optional)
dataset/
*.jpg
*.png
*.jpeg
!homepage.jpg
!hardware-setup.jpg
!detection-results.jpg
!twilio-alerts.jpg
!maintenance-record.jpg
!team-photo.jpg
